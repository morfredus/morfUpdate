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

L'API d'écriture écoute par défaut uniquement sur `127.0.0.1:8794`. Chaque
requête `POST` doit présenter le jeton configuré dans l'en-tête HTTP :

```text
Authorization: Bearer <jeton>
```

Un jeton absent, invalide ou une requête non locale reçoit `401` ou `403`.
L'exposition à une autre machine est hors du premier contrat : elle exigera un
jumelage ou mTLS, jamais la réutilisation d'un bearer token sur le LAN.

## Configuration initiale

Le paquet démarre avec une liste `targets` vide : il répond à `/healthz` mais
refuse toute installation. Ajouter ensuite, dans la configuration administrée,
le fichier de jeton GitHub en lecture seule et chaque cible autorisée. Une cible
déclare son projet, son unité, son dépôt, son URL `/healthz` et, sous Windows,
son répertoire applicatif ainsi que `service_manager` (`scm` ou `task`).

Sous Windows, le jeton est placé sous
`%ProgramData%/morfsystem/morfupdate/state/` et son ACL est limitée à `SYSTEM`.
Les tâches de service morfUpdate et morfMonitor exécutées sous cette identité
peuvent donc collaborer sans rendre ce secret lisible par un utilisateur.

## Demander une mise à jour

```http
POST /api/v1/updates
Authorization: Bearer <jeton>
Content-Type: application/json

{
  "project": "morfCollector",
  "version": "0.7.0"
}
```

La demande ne contient ni URL, ni chemin, ni commande. `project` doit être une
entrée déclarée dans la configuration locale de l'agent. `version` doit désigner
une release publiée de ce projet. L'agent refuse sa propre mise à jour dans ce
premier jalon.

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
Authorization: Bearer <jeton>
```

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

États finaux : `succeeded`, `rejected`, `failed`.

L'opération est conservée dans le répertoire d'état de l'agent. Un échec garde
son diagnostic réel. Aucun rollback automatique n'est annoncé.

## Validation obligatoire

Avant toute élévation de privilèges, l'agent contrôle :

1. la release source et son tag `vX.Y.Z` ;
2. l'asset correspondant exactement à l'OS et à l'architecture locales ;
3. `manifest.json`, le nom canonique de l'asset et son SHA-256 ;
4. le commit de provenance de l'asset et sa correspondance avec le tag source ;
5. l'appartenance du projet à la liste locale autorisée.

Sous Linux, le helper privilégié est un second exécutable, installé hors de
`/opt`, appartenant à `root` et seulement exécutable par le compte morfUpdate.
Il accepte exclusivement `--install-deb <artifact> <service>` : l'artefact doit
être un `.deb` situé sous le répertoire de téléchargements propre à l'agent et
le service doit figurer dans la configuration root-owned. Il ne reçoit ni
données HTTP, ni URL, ni commande.

## Exécuteurs

| Élément | Linux | Windows |
| --- | --- | --- |
| Asset | `.deb` | `.zip` |
| Service | systemd | Service Control Manager, ou tâche système historique |
| Installation | `dpkg` avec dépendances résolues | extraction contrôlée et remplacement applicatif |
| Configuration et données | conservées hors du paquet | conservées hors du ZIP |
| Contrôle final | unité active et `/healthz` | SCM actif et `/healthz` |

L'exécuteur ne réenregistre un service que si le contrat du paquet le requiert.
Il n'exécute jamais aveuglément un script contenu dans une archive.
