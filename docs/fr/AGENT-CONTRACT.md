# Contrat de l'agent morfUpdate

## Responsabilité

`morfUpdate` applique, sur sa propre machine, une mise à jour explicitement
demandée d'un service morfSystem. Il ne décide jamais qu'une mise à jour doit
avoir lieu et ne pilote aucune autre machine.

L'agent conserve la bibliothèque de comparaison de versions. Le service ajoute
une file d'opérations persistante et deux exécuteurs : Linux/systemd et Windows
SCM. Les deux suivent exactement le même contrat HTTP et les mêmes contrôles de
provenance.

## Accès et authentification

L'API écoute uniquement sur `127.0.0.1:8794` : seule la machine elle-même peut
l'atteindre. Depuis la 0.4.0, il n'y a **plus de jeton** : le périmètre de
sécurité est la boucle locale, plus la liste blanche des cibles, la provenance,
le SHA-256 et la plateforme, tous contrôlés à chaque demande. En pratique, c'est
morfMonitor, sur la même machine, qui relaie les demandes venues de son
interface Web (`POST /api/updates`, `POST /api/restart`).

`GET /healthz` et `GET /status` restent lisibles sans condition : un superviseur
local doit pouvoir distinguer un agent arrêté d'un agent qui refuse une demande.

L'exposition à une autre machine est hors contrat : elle exigera un jumelage ou
mTLS, jamais un secret partagé sur le LAN.

## Configuration initiale

Le paquet démarre avec une liste `targets` qui autorise **tous les services
du parc**. `morfUpdate` lui-même n'est accepté que si sa cible porte
`"self": true` (voir « Auto-mise à jour »).
Chaque cible déclare son projet GitHub, son unité, son dépôt, son URL
`/healthz` et, sous Windows, `app_dir` plus `service_manager`.

`service.py update` n'ajoute **jamais** d'entrée dans une liste déjà
présente. Une machine qui n'avait qu'une cible de test (souvent
`morfCollector`) garde cette liste jusqu'à un `config push --force`.

## Demander une mise à jour

```http
POST /api/v1/updates
Content-Type: application/json

{
  "project": "morfCollector",
  "version": "0.7.0"
}
```

La demande ne contient ni URL, ni chemin, ni commande. `project` doit être une
entrée déclarée dans la configuration locale de l'agent. `version` doit désigner
une release publiée de ce projet. Une demande visant `morfUpdate` reçoit
`409 Conflict` tant que sa cible n'est pas déclarée `"self": true`.

Réponse immédiate :

```http
202 Accepted
Content-Type: application/json

{
  "id": "…",
  "state": "queued"
}
```

Une seule opération peut être active par machine. Une seconde demande reçoit
`409 Conflict` avec l'identifiant et l'état de l'opération active.

## Consulter une opération

```http
GET /api/v1/updates/<id>
```

Le journal est commun aux mises à jour et aux relances : `GET /api/v1/restart/<id>`
renvoie la même chose.

```json
{
  "id": "…",
  "project": "morfCollector",
  "from_version": "0.5.1",
  "to_version": "0.7.0",
  "platform": "linux-arm64",
  "state": "verifying",
  "created_at": "2026-08-20T12:00:00Z",
  "updated_at": "2026-08-20T12:00:03Z",
  "detail": "SHA-256 verified"
}
```

États transitifs : `queued`, `downloading`, `verifying`, `installing`,
`restarting`, `health_check`.

États finaux : `succeeded`, `rejected`, `failed`, et pour l'auto-mise à jour
`rolled_back`.

États propres à l'auto-mise à jour : `rollback_prepared` (nouveau paquet et paquet
de retour vérifiés et mis de côté), puis `delegated` (la main est passée à
l'applieur détaché).

L'opération est conservée dans le répertoire d'état de l'agent. Un échec garde
son diagnostic réel. Pour une mise à jour ordinaire, aucun retour arrière
automatique n'a lieu ; l'auto-mise à jour et `source-bundle` en prévoient un.

## Relancer un service

```http
POST /api/v1/restart
Content-Type: application/json

{ "project": "morfCollector" }
```

Action bornée, sans version ni source : `project` doit être une cible déclarée,
et le nom de l'unité systemd vient de la configuration, jamais du client. Elle
partage le verrou des mises à jour (`409` si une opération est active) et se suit
par son identifiant. Linux seulement pour l'instant.

## Stratégies d'installation

Le `manifest.json` de la release déclare un bloc `install` :

- `package` (défaut, rétro-compatible) : l'asset est un paquet compilé (`.deb` ou
  `.zip`), installé comme décrit dans « Exécuteurs ».
- `source-bundle` (projet non compilé, par exemple morfDashboard) : l'asset est
  une archive `.tar.gz` des fichiers applicatifs. L'agent l'inspecte (aucun chemin
  absolu ni `..`), l'extrait **sans privilège**, vérifie que son fichier `VERSION`
  correspond à la version demandée, puis confie au helper l'échange atomique du
  répertoire applicatif, avec retour arrière. `/etc` et `/var/lib` sont préservés.
  Linux seulement.

Un asset dont le format ne correspond pas à la stratégie déclarée est refusé.

## Auto-mise à jour (Linux, désactivée par défaut)

Un agent ne peut pas constater lui-même le succès de son propre remplacement. Une
cible `morfUpdate` déclarée `"self": true` déclenche donc une succession en deux
temps :

1. l'agent télécharge et vérifie le nouveau paquet **et** le paquet de la version
   en cours (le retour), persiste l'opération en `delegated`, puis lance un
   applieur détaché (`systemd-run`, hors du cgroup de morfupdate) ;
2. l'applieur arrête, installe, redémarre et interroge `/healthz` ; si la nouvelle
   version ne répond pas, il réinstalle l'ancienne ;
3. au démarrage, l'agent qui tourne (nouveau ou ancien) solde l'opération d'après
   sa propre version : `succeeded` ou `rolled_back`.

Ce chemin n'est pas encore validé sur un vrai systemd : le garder sur le banc de
test (pi4dev) tant qu'il n'est pas éprouvé. Non disponible sous Windows.

## Validation obligatoire

Avant toute élévation de privilèges, l'agent contrôle :

1. la release source et son tag `vX.Y.Z` ;
2. l'asset correspondant exactement à l'OS et à l'architecture locales ;
3. `manifest.json`, le nom canonique de l'asset et son SHA-256 ;
4. le commit de provenance de l'asset et sa correspondance avec le tag source ;
5. l'appartenance du projet à la liste locale autorisée.

Sous Linux, le helper privilégié est un second exécutable, installé hors de
`/opt`, appartenant à `root` et seulement exécutable par le compte morfUpdate.
Il n'accepte qu'une liste fermée de verbes : `--install-deb <artefact> <service>`,
`--install-bundle <répertoire> <service>`, `--restart <service>`, et pour
l'auto-mise à jour `--self-apply` / `--self-apply-run`. Les fichiers doivent se
trouver sous le répertoire de téléchargements propre à l'agent, et le service
doit figurer dans la configuration root-owned. Il ne reçoit ni données HTTP, ni
URL, ni commande.

## Exécuteurs

| Élément | Linux | Windows |
| --- | --- | --- |
| Asset | `.deb` | `.zip` |
| Service | systemd | Service Control Manager, ou tâche système historique |
| Installation | `dpkg` avec dépendances résolues | extraction contrôlée et remplacement applicatif |
| Configuration et données | conservées hors du paquet | conservées hors du ZIP |
| Contrôle final | unité active et `/healthz` (retries) | SCM actif et `/healthz` (retries) |

Après installation, `/healthz` est interrogé plusieurs fois (environ une minute).
Si le paquet est déjà posé et le service relancé, un `/healthz` encore muet ne
fait pas échouer l'opération : la version demandée est considérée installée.

L'exécuteur ne réenregistre un service que si le contrat du paquet le requiert.
Il n'exécute jamais aveuglément un script contenu dans une archive.
