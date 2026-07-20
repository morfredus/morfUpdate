# Intégrer morfUpdate dans une application

Exemple pour **SiteWatch** et **ComponentHub**. Identique pour tout projet Qt.

## 1. Lier la bibliothèque

Dans le `CMakeLists.txt` de l'application :

```cmake
# Dossier voisin (ou third_party/morfUpdate en copie interne)
add_subdirectory(../morfUpdate morfUpdate EXTERNAL_SOURCE)

target_link_libraries(SiteWatch PRIVATE
    morfUpdate::morfUpdate   # coeur (obligatoire)
    morfUpdate::Widgets         # dialogue (si UI souhaitee)
)
```

Si l'application n'a pas d'interface, ne lier que `morfUpdate::morfUpdate`
et se passer de `Widgets` (mettre `-DMORFUPDATE_BUILD_WIDGETS=OFF`).

## 2. Disposer de la version courante

SiteWatch définit déjà `SITEWATCH_VERSION`. Pour ComponentHub, ajouter dans son
CMake (comme SiteWatch le fait) :

```cmake
target_compile_definitions(ComponentHub PRIVATE
    COMPONENTHUB_VERSION="${PROJECT_VERSION}")
```

## 3. Vérification automatique au démarrage

Dans `MainWindow`, après la construction de l'UI, une seule ligne :

```cpp
#include <morfupdate/UpdateDialog.h>

void MainWindow::checkForUpdates(bool manual) {
    morfupdate::morfUpdateConfig cfg;
    cfg.owner = "morfredus";
    cfg.repo  = "SiteWatch";                 // ou "ComponentHub"
    cfg.currentVersion = SITEWATCH_VERSION;
    cfg.includePrereleases = false;

    // manual == true (menu « Rechercher les mises a jour ») : on montre aussi
    // « a jour » et les erreurs. Au demarrage : silencieux.
    morfupdate::checkAndNotify(this, "SiteWatch", cfg, /*silentIfUpToDate=*/!manual);
}
```

Appeler `checkForUpdates(false)` une fois dans le constructeur (idéalement
derrière une préférence « vérifier au démarrage »), et `checkForUpdates(true)`
depuis une entrée de menu **Aide → Rechercher les mises à jour**.

> `checkAndNotify` est **non bloquant** : la fenêtre s'affiche immédiatement, la
> notification apparaît quand la réponse réseau arrive.

## 4. Dépôts privés / quota API

Si le dépôt est privé ou pour éviter la limite anonyme de l'API GitHub, fournir
un jeton à portée `repo` en lecture seule :

```cpp
cfg.githubToken = /* jeton lu depuis la config, jamais code en dur */;
```

Ne jamais committer de jeton : le lire depuis la configuration de l'application
(SiteWatch a déjà un `config.json`).

## 5. Basculer sur une source non-GitHub (plus tard)

Pour héberger un manifeste sur o2switch au lieu de GitHub, implémenter
`IUpdateSource` (une classe, une méthode `fetchLatest`) et passer l'instance au
constructeur enfichable :

```cpp
auto* source = new MonManifesteSource(cfg, this);
auto* checker = new morfupdate::UpdateChecker(cfg, source, this);
```

Le reste (comparaison, dialogue) ne change pas.

## 6. Vérifier

```sh
# Contre le vrai depot, en simulant une vieille version installee :
./build-mingw/examples/minimal/morfupdate_demo github morfredus SiteWatch 0.0.1
# -> MISE A JOUR DISPONIBLE : 1.4.2
```
