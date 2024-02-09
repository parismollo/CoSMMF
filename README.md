# PSAR
## Partage cohérent de fichiers mappés en mémoire
L’appel système mmap permet de projeter un fichier en mémoire pour faciliter son utilisation et son partage,
on dit que le fichier est « mappé » en mémoire. Le contenu du fichier peut être directement lu ou écrit en
lisant ou modifiant la zone de mémoire virtuelle où a été « mappé » le fichier.
L’objectif de ce projet est de gérer l’accès concurrent à un fichier mappé. Plusieurs processus peuvent
mapper le même fichier en appelant mmap avec l’option MAP_SHARED. Tous liront alors le même contenu
et les écritures d’un processus sont visibles par tous, ce qui peut entraîner des incohérences. On souhaite
dans ce projet que les écritures ne soient visibles que par leur écrivain en implémentant un mécanisme de
copie-sur-écriture.
La figure illustre le fonctionnement de ce qui doit être réalisé.
En (1), tant que les accès aux pages du fichier sont en lecture, tous les processus accèdent directement au
même contenu du fichier. Initialement le fichier est mappé uniquement avec les droits de lecture.
En (2), lorsqu’un processus P1 ou P1 accède en écriture, cela déclenche le signal SIGSEGV. Le handler du
signal, copie alors la page modifiée à une autre emplacement et redonne les droits lecture écriture à la page
grâce à l’appel système mprotect. Pour que l’accès puisse se faire au nouvel emplacement, il faut alors
modifier dans la table des pages l’adresse physique de la page modifiée.
L’ensemble des pages modifiées sont stockées dans un fichiers log associé à chaque processus, le fichier
initial n’est jamais modifié. A la fin une commande merge qui prend en paramètre le fichier initial et un log
permet de construire une nouvelle version du fichier.

Prérequis, ce projet devra être réalisé sur un linux sur processeur x86_64 ou ARM, vous serez amenés à
modifier directement les tables des pages en utilisant l’outil pteditor
(https://github.com/misc0110/PTEditor).
