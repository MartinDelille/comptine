---
layout: page
lang: fr
title: Fonctionnalités
permalink: /fr/features/
---

## Gestion des fichiers

- **Nouveau** : Créez un nouveau budget vide (Fichier > Nouveau ou Cmd+N)
- **Ouvrir** : Chargez un fichier budget au format YAML (Fichier > Ouvrir ou Cmd+O)
- **Enregistrer** : Sauvegardez votre budget (Fichier > Enregistrer ou Cmd+S)
- **Enregistrer sous** : Sauvegardez vers un nouveau fichier (Fichier > Enregistrer sous ou Cmd+Shift+S)
- **Fichiers récents** : Accès rapide aux derniers fichiers ouverts

## Gestion des comptes

- **Comptes multiples** : Gérez plusieurs comptes dans un seul fichier budget
- **Sélecteur de compte** : Changez de compte facilement via le menu déroulant
- **Renommer** : Renommez vos comptes selon vos besoins

## Opérations

- **Liste des opérations** : Visualisez toutes les opérations triées par date
- **Détails** : Consultez les informations détaillées de chaque opération
- **Sélection multiple** : Clic simple, Ctrl+clic, Shift+clic, Cmd+A
- **Navigation clavier** : Flèches haut/bas pour naviguer
- **Solde courant** : Calcul automatique du solde pour chaque opération
- **Copier** : Copiez les opérations sélectionnées au format CSV (Cmd+C)
- **Modifier** : Éditez les détails via le bouton édition ou Ctrl+E
  - Montant, date, date budget
  - Répartition sur plusieurs catégories
  - Toutes les modifications sont annulables

## Import CSV

- **Import facile** : Importez vos relevés bancaires (Fichier > Importer CSV)
- **Détection automatique** : Détecte le délimiteur et l'encodage
- **Mapping des colonnes** : Reconnaissance automatique des colonnes
- **Banques françaises** : Compatible avec le format des banques françaises
- **Détection des doublons** : Évite les imports en double
- **Catégories optionnelles** : Importez les catégories depuis le CSV

## Vue Budget

- **Budget mensuel** : Visualisez votre budget mois par mois
- **Suivi par catégorie** : Suivez vos dépenses par rapport aux limites définies
- **Navigation temporelle** : Naviguez entre les mois pour consulter l'historique
- **Restes accumulés** : L'affichage du budget montre les montants reportés

## Gestion des restes

Gérez le budget non dépensé à la fin de chaque mois :

- **Dialogue des restes** : Accédez via le bouton "Restes..." dans la Vue Budget
- **Décisions par catégorie** : Pour chaque catégorie avec un reste, choisissez de :
  - **Épargner** : Transférez le reste vers votre épargne personnelle
  - **Reporter** : Reportez pour augmenter le budget du mois suivant
- **Restes accumulés** : Les montants reportés s'ajoutent aux limites des mois futurs
- **Résumé mensuel** : Consultez les totaux d'épargne et de transferts
- **Annulable** : Toutes les décisions de restes supportent annuler/refaire

## Annuler / Refaire

- **Annuler** : Annulez la dernière action (Édition > Annuler ou Cmd+Z)
- **Refaire** : Rétablissez l'action annulée (Édition > Refaire ou Cmd+Shift+Z)

### Actions annulables

- Renommage de compte
- Import CSV
- Modification de catégorie
- Affectation de catégorie
- Répartition d'opération
- Modification de montant, date, date budget
- Décision de reste

## Interface utilisateur

- **Onglets** : Basculez entre Opérations et Budget
- **Thèmes** : Clair, Sombre ou Système
- **Langues** : Français et Anglais
- **Préférences** : Configurez l'application (Cmd+,)

## Mises à jour

- **Vérification automatique** : Recherche de nouvelles versions au démarrage
- **Vérification manuelle** : Aide > Rechercher des mises à jour
- **Notes de version** : Consultez les nouveautés
- **Lien de téléchargement** : Accès direct aux releases GitHub

## Format de données

- **Stockage YAML** : Format lisible et éditable manuellement
- **Précision** : Montants stockés avec 2 décimales
