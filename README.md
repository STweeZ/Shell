**Pourcentage de la part de travail :**

Delacroix Grégoire : 50%

Avot Kévin : 50%

**Fonctionnalités implémentées :**

    - Séquencement : ";", "&&", "||".
    - Wildcards et jokers.
    - Changement de répertoire avec cd avec ~ pris en charge et répertoire optionnel.
    - Ctrl-C qui demande confirmation avant de quitter, et tue tous les processus.
    - Ctrl-C qui, lors d'une exécution en foreground, est propagée au processus en cours d'exécution.
    - status pour les codes de retours du dernier processus en foreground.
    - Exécution possible en background avec le symbole "&". Numéro de job affiché et réinitialisé quand plus aucun job.
    - Informations du processus en bakcground quand il se termine.
    - myjobs affiche la liste des processus en background.
    - Ctrl-Z pour stopper la commande en cours. Il devient un job.
    - Variables locales. set et unset.
    - Utilisation des variables locales avec "$". set pour afficher l'ensemble des variables locales.
    - Toutes les redirections avec chevrons sauf <.
    - Commande myls avec tous ses paramètres ainsi que les couleurs pour les différents types de fichier avec la prise en charge de multiples arguments/paramètres et de ~.
    - Gestion des pipelines mais avec une limite de un | par séparateur(&&, ||, ;).

**Fonctionnalités non implémentées :**

    - Les commandes myfg et mybg.
    - Variables d'environnement. set et setenv et donc mémoire partagée.
    - La commande myps.
    - glob et pipe ne sont pas fonctionnels avec myls.
    - La compatibilité entre diverses fonctionnalités comme les | et les >.
    - Les pipelines à l'infini.
    - La redirection <.

**Bugs :**

    - La commande set ne peut s'exécuter qu'en foreground, car elle fonctionne par le biais d'un pipe et le père attend une lecture dans cette dernière.
    - Ctrl-c tue les processus fils avant de demander à l'utilisateur. Le signal est propagé du processus père au fils.
    - Si on essaye d'éxecuter une commande contenant plus d'un pipe par séparateur le programme bouclera à l'infini car le dernier fils est bloqué et donc le père aussi car il attend ses fils.
    - Si on essaye de ctrl-c afin de quitter le shell lors du bug ci-dessus une erreur lié au free se produira lors de la fermeture du shell.
