# TODO — Corrections Server.cpp / ListeningSocket.cpp / Dépendances

## Étape 1 : `srcs/server/Server.cpp` — Boucle pollLoop()
- Corriger `for(size_t i = 0; i < count; i++)` → `i < fds.size()`
- Réorganiser la branche `if (i < _listeningSockets.size())` / `else` pour traiter les clients

## Étape 2 : `srcs/server/Server.cpp` — Timeouts & guard empty sockets
- Décommenter `checkTimeouts()` dans `pollLoop()`
- Ajouter un guard `if (_listeningSockets.empty())` après `setupListeningSockets()` pour éviter le busy-wait

## Étape 3 : `srcs/server/Server.cpp` — Pipeline HTTP
- Dans `handleClientRead()` : alimenter `c->getRequest().appendData()`, vérifier `isComplete()`, choisir `ServerConfig` via `Host`, appeler `RequestHandler::handle()`, sérialiser la réponse dans `getWriteBuffer()`, passer à `WRITING`

## Étape 4 : `srcs/server/Server.cpp` — Fermeture connexion
- Dans `handleClientWrite()` : si `getWriteBuffer().empty()` après envoi, passer à `CLOSING` ou appeler `closeClient()`

## Étape 5 : `srcs/server/Server.cpp` — Robustesse socket client
- Dans `acceptNewConnection()` : `fcntl(clientfd, F_SETFL, O_NONBLOCK)` + gestion erreur

## Étape 6 : `srcs/server/Server.cpp` — Logs & signature
- Corriger les logs qui s'affichent même après `closeClient()` sur erreur fatale
- `checkDuplicate()` : passer le vector par `const&`

## Étape 7 : `srcs/server/ListeningSocket.cpp` — fcntl check
- Vérifier le retour de `fcntl(sockfd, F_SETFL, O_NONBLOCK)`

## Étape 8 : `includes/server/ListeningSocket.hpp` — Cleanup includes
- Déplacer les includes système (`<sys/socket.h>`, etc.) dans le `.cpp`

## Étape 9 : `srcs/http/HttpResponse.cpp` — Typo
- Corriger `"Content-Lenght"` → `"Content-Length"` (2 occurrences)

## Étape 10 : `includes/server/Server.hpp` — Commentaire obsolète
- Supprimer ou mettre à jour le commentaire `run() is currently a no-op stub`

