# ZiPulses
Compteur d'impulsions communiquant en ZigBee 3.0.  
Le ZiPulses est disponible en boutique :  
[Boutique LiXee](https://lixee.fr/)

## Sommaire

* [Description](#description)  
* [Fonctionnement](#fonctionnement)
   * [Appairage](#appairage)
   * [le capteur est appairé](#le-capteur-est-appair%C3%A9)
* [Boitier](#boitier)
* [PCB](#pcb)
* [Installation](#installation)
   * [Alimentation](#alimentation)
   * [Impulsions](#impulsions)
* [Voyant lumineux](#voyant-lumineux)
* [MAJ non OTA](#mise-%C3%A0-jour-du-firmware-non-ota)
* [Clusters](#clusters)
   * [Clusters list](#clusters-list)
   * [Basic Cluster](#basic-cluster-0x0000)
   * [PowerConfiguration Cluster](#powerconfiguration-cluster-0x0001)
   * [TemperatureMeasurement Cluster](#temperature-measurement-0x0402)
   * [SimpleMetering Cluster](#simple-metering-0x0702)
   * [Tableau unité](#tableau-dunité-de-mesure)
* [Intégrations](#intégrations)
   * [Jeedom](#jeedom)
   * [Domoticz](#domoticz)
   * [HomeAssistant](#home-assistant)
   * [Eedomus](#eedomus)
* [Changelog](#changelog)

## Description

L'appareil **ZiPulses** permet de récupérer les impulsions d'un compteur pour créer un index et le transmettre à un coordinateur Zigbee.  
**ZiPulses** est un capteur Zigbee de type "ZED" (Zigbee End Device). Il ne peut donc pas router les informations des autres capteurs et nécessite des périodes d'endormissement afin de préserver sa consommation d'énergie.  
  
**ZiPulses** peut être alimenté de deux façons:  
* avec une pile CR2450 (3V)
* avec une alimentation externe de 5V à 12V

### Fonctionnement

#### Appairage

##### Si le produit n'est associé à aucun réseau zigbee
* ZiPulses se mettra en mode appairage dès la mise sous tension. (la Led clignote 3 sec)
* ZiPulses se mettra en mode appairage à chaque appuie sur le bouton "Link". (la Led clignote 3 sec)

##### Si le produit est déjà associé à un réseau zigbee
* ZiPulses fera un "rejoin" à chaque mise sous tension.
* Pour sortir d'un réseau, il faut rester appuyer sur le bouton "Link" pendant 10 sec. Il se remettra automatiquement en mode appairage. (la Led clignote 3 sec)

#### Le capteur est appairé

##### Juste après l'appairage ou Rejoin
* Le capteur ZiPulses reste éveillé environ 10 secondes afin que le coordinateur zigbee puisse faire un bind sur les clusters : 0x0001, 0x0402, 0x0702 ou interroger le capteur

##### ZiPulses n'est pas sollicité
* Le capteur va se réveiller et s'endormir toutes les 60 secondes
* Le capteur envoie toutes les 2 heures, la tension de batterie, la temperature et le dernier index

##### ZiPulses reçoit des impulsions
* Le capteur est reveillé. Il incrémente l'index et l'envoie au coordinateur

⚠️ **Pour que des requêtes manuelles du coordinateur vers le ZiPulses soit prises en compte, il est indispensable d'appuyer sur le bouton "Link" (1 seule fois) juste après le lancement de la requête ZigBee (afin de le réveiller). Cette technique est notamment indispensable pour modifier le coefficient multiplicateur de l'index et/ou l'unité de mesure par exemple.**  

## Boitier

### Photos 
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_antenne_face.jpg" width="800" title="test">  
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_antenne.jpg" width="800">  
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_without_antenne.jpg" width="800">  
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_without_antenne_cache.jpg" width="800">  

### Exemple de schema montage
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_schema.jpg" width="800">  

## PCB

Les PCB ci-dessous ne sont pas forcément contractuels. Ils peuvent légèrement être modifiés en fonction des évolutions mais les fonctions seront similaires

### Face 1

<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/face1_components.jpg" width="400">  
  
Magenta : MCU zigbee  
Bleu : régulateur 3.3V  
Jaune : Connecteur antenne externe

### Face 2

<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/face2_legend.jpg" width="400">  


## Installation

### Alimentation
Comme vu plus haut, il existe 2 modes d'alimentation qui ne peuvent fonctionner en même temps.  

#### Alimentation externe
Par défaut, l'appareil est en mode alimentation externe. 
Tout d'abord, il faut vérifier que le Switch alimentation (visible [ici](#face-2)) est bien positionné sur **REG** (comme régulateur)  
Ensuite, il faut brancher sur les connecteurs d'alimentation noté **VIN** et **GND**, les fils d'alimentation.  
⚠️ **La tension aux bornes doit être comprise entre 5 et 12 VDC.**

⚠️  **A partir de la version 2 (v0.7 sur PCB), il est possible d'utiliser les bornes pour brancher des piles (pas de surconsommations lié au régulateur)**  

#### Alimentation sur pile

Tout d'abord, il faut vérifier que le Switch alimentation (visible [ici](#face-2)) est bien positionné sur **BAT** (comme Battery)  
Ensuite, il faut introduire une pile de type CR2450 dans l'emplacement en respectant la polarité. (+ en haut et - en bas)

#### Alimentation externe (custom) - pour la V0.5

Il y a une possibilité d'utiliser les plots de programmation pour alimenter le ZiPulses. Vous pouvez utiliser un autre support de pile branché sur le plot 3v3 et GND du port de programmation. Dans ce mode là, a position du switch REG/BAT n'a pas d'incidence.

Exemple de piles avec support : 
* Soit 2 piles LR6 AA en série (env 1500mAh) [support](https://www.lextronic.fr/boitier-coupleur-pour-2-piles-aaa-lr3-64605.html)   
* Soit 2 piles LR14 en série (env 8000mAh)  [support](https://www.conrad.fr/fr/p/support-de-pile-2x-lr14-c-goobay-45834-raccordement-a-souder-l-x-l-x-h-62-x-56-x-23-mm-615668.html?refresh=true)  
* Soit 2 piles LR20 en série (env 16000 mAh)  [support](https://fr.aliexpress.com/item/32235908591.html)  
* Soit une pile CR123A (env 1500mAh)  [support](https://fr.aliexpress.com/item/32901118515.html)  
Il y a surement d'autres techno/type de piles mais j'ai mis les plus usuelles / moins chères. Il faut juste respecter la tension de 3V à 3.6V **MAX**  
Bien entendu, les capacités dépendent des marques, de l'environnement et du type d'utilisation. Il est conseillé de prendre 70-80% de la vraie valeur.  

![1693322718749](https://github.com/fairecasoimeme/ZiPulses/assets/22256438/07c31ca3-568d-4eac-a330-a3a838c7d936)  
                                                                                       _Exemple de montage_

Sachant que la pile CR2450 d'origine contient environ 500mAh, vous pouvez calculer (en fonction de vos utilisations) l'autonomie du ZiPulses avec vos nouvelles piles

⚠️  **Avec la version 2 (v0.7 sur PCB), il est désormais possible d'utiliser ces supports avec les bornes classiques VIN et GND de l'appareil**

### Impulsions
Le ZiPulses est en mesure de détecter les impulsions (créneau bas) à partir de 20ms environ. (à partir de la V5.0)

#### Compteur GAZPAR
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_compteur_gazpar.JPG" width="400">  
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_gazpar_JAE.jpg" width="400">  

⚠️ **Attention, le ZiPulses n'a pas le marquage ATEX. Pour des raisons de sécurité, il faut utiliser un connecteur JAE marqué compatible Gazpar. Il est aussi recommandé de placer le ZiPulses à l'extérieur du coffret où se trovue le compteur Gaz.**  


#### Compteur / débimètre eau  

##### Avec sortie impulsion de type ILS (lamelles métallique)
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_compteur_eau.JPG" width="400">  

##### Avec sortie impulsion (Transistor ou Mosfet)
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_compteur_counter_mosfet.jpg" width="400">  

## Voyant lumineux

Voici les différentes possibilités :  

### La LED bleue est éteinte 

* Le capteur est endormi ou non sollicité

### La LED bleue s'allume puis s'éteint 
* le capteur a reçu une impulsion
* le capteur est appairé et le bouton link a été actionné.

### La LED bleue clignote 
* le capteur n'est pas encore appairé

### La LED bleue est allumé constamment
* le bouton "Link" est appuyé sans être relaché
* le capteur est planté et nécessite un reset

## Mise à jour du Firmware (non OTA)

Tout d'abord, il faut dévisser le boitier afin de sortir la carte électronique.
Ensuite, il faut brancher le module USB TTL (CP2102 dans l'exemple) sur le **ZiPulses** comme sur la photo. 

<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_update.jpg" width="400">  
<img src="https://github.com/fairecasoimeme/ZiPulses/blob/master/Doc/photos/ZiPulses_update_legend.jpg" width="400"> 

Une fois que les branchements sont OK, il suffit d'insérer sur votre ordinateur la clef USB en maintenant le bouton **Flash** puis relacher.
L'utilisation d'une rallonge USB peut se révéler plus pratique pour faire la mise à jour.

Enfin vous pouvez suivre les [instructions suivantes](https://zigate.fr/documentation/mise-a-jour-de-la-zigate-2/) (similaire à la mise à jour d'une ZiGate+ (V2))

## Clusters

### Clusters List

|Num (Hexa)|Name|I/O| Comment|
|----------|----|---|--------|  
|0x0000|Basic|I||
|0x0001|PowerConfiguration|I| Doit être "bind" pour recevoir les trames|
|0x0003|Identify|I||
|0x0402|Temperature Measurement|I|Doit être "bind" pour recevoir les trames|
|0x0702|Simple Metering|I|Doit être "bind" pour recevoir les trames|

### Basic Cluster (0x0000)

|attribut|Name|Value|
|------|----------|------|
|0x0000 |ZCLVersion|0x0003|  
|0x0001 | ApplicationVersion|0x0001|  
|0x0002 | StackVersion|0x0002|
|0x0003 | HWVersion|0x0001|
|0x0004 | ManufacturerName|LiXee|
|0x0005 | ModelIdentifier|ZiPulses|
|0x0006 | DateCode|20221110|
|0x0007 | PowerSource|0x03|
|0x4000 | SWBuildID| 4000-01|

### PowerConfiguration Cluster (0x0001) 
|attribut|Name|Right|Value|
|------|----------|------|------|
|0x0020 | BatteryVoltage|RO|| 
|0x0021 | BatteryPercentageRemaining|RO| (From V6)| 

### Temperature Measurement (0x0402)

La temperature n'est pas précise mais elle peut être utile pour prévenir du gel. (surtout pour les compteurs d'eau)
La valeur est en centième de degré.
|attribut|Name|Right|Value|
|------|----------|------|------|
|0x0000 |Temperature|RO|xxxx °c|  

### Simple Metering (0x0702) 
|attribut|Name|Right|Type|Value|Comment|
|------|----------|------|------|------|------|
|0x0000 | CurrentSummationDelivered|RO|Uint48| | Index principal (nombre d'impulsions enregistrés uniquement)|
|0x0100 | CurrentTier1SummationDelivered|RO|Uint48| | Index calculé (nombre d'impulsions X coefficient multiplicateur) ou (nombre d'impulsions / coefficient diviseur) en fonction des paramétrages|
|0x0300 | unitOfMeasure|RW|Enum8| 0 (kWh) par defaut |Unité de mesure pour les impulsions. Pour choisir la bonne unité de measure cf [Tableau de mesures](#tableau-dunit%C3%A9-de-mesure)|
|0x0301 | Multiplier|RW|Uint24| 1 par defaut | Coefficient multiplicateur pour les impulsions|
|0x0302 | Divisor|RW|Uint24| 1 par defaut | Coefficient diviseur pour les impulsions|

#### Tableau d'unité de mesure
|value|Description|
|------|----------|
|0|kWh|
|1|m3|
|2|ft3|
|3|ccf|
|4|US gl|
|5|IMP gl|
|6|BTUs|
|7|L (litre)|
|8|kPA (jauge)|
|9|kPA (absolu)|
|10|kPA (absolu)|
|11|sans unité|
|12|MJ|
|13|kVar|
|14 ... 127|Réservé|

## Intégrations 

### Jeedom  

Compatible avec le plugin zigbee.

### Domoticz  

Compatible avec le plugin Zigbeefordomoticz en version 7.1.003 (et ultérieures)
- Device de type compteur (permettant de choisir le type et donc l'unité, mais aussi l'offset, le diviseur,...)
- Remonte également un device température et un device tension de la pile/alim externe, mis à jour toutes les deux heures (indiquera donc 0°C et 0V pendant les deux premières heures)

### Home-assistant

#### ZHA

Nativement compatible avec ZHA.

⚠️ Pour changer les coefficients multiplicateurs / diviseurs et l'unité de mesure il faut suivre la procédure suivante :  

1- Sélectionner l'appareil puis lancer l'outil de gestion de l'appareil.  
![image](https://user-images.githubusercontent.com/22256438/229559210-a08427b8-e95a-4a68-96fc-da6452c1290b.png)  

2- Sélectionner le cluster **"Metering"** et l'attribut correspondant à **"Multiplier"**, **"Divisor"** ou **"unit_of_measure"**  
![image](https://user-images.githubusercontent.com/22256438/229560149-3be40328-a486-42f1-b38f-953c2d8d829f.png)

3- Inscrire la valeur souhaitée et appuyer sur **"Ecrire l'attribut"**.  
4- Enfin, Appuyer sur le bouton "Link" du ZiPulses pour réveiller l'appareil pour l'enregistrement.  

#### Z2M

Nativement compatible depuis la version [1.30.3-1](https://github.com/Koenkk/zigbee2mqtt/releases/tag/1.30.3)

⚠️ Pour changer les coefficients multiplicateurs / diviseurs et l'unité de mesure il faut suivre la procédure suivante :  
  
1- Dans l'onglet **"Expose"**, sélectionner le bon coefficient et dans l'instant, appuyer sur le bouton "link" du ZiPulses.  
2- Vérifier que le coefficient a bien été enregistré en changeant d'onglet ou en rafraichissant la page.  
3- Si le coefficient a bien été validé, il faut aller dans l'ongle **"A propos"** puis cliquer sur le logo jaune de reconfiguration.  
![image](https://user-images.githubusercontent.com/22256438/229558488-f8e09263-ede3-404c-b1c0-d7eb6d0d3bb4.png)

4- Une fois validé, vous devez voir apparaître **"configuring"** en vert en haut à droite.  
5- Il faut ensuite appuyer sur le bouton "Link" du ZiPulses et vous devriez voir **"successfully configured"**  
![image](https://user-images.githubusercontent.com/22256438/229558046-7f50f0c5-f575-4f76-a7df-8d29b627130c.png)

Si rien n'apparait, redémarrer zigbee2mqtt et refaire la procédure    
  
Normalement, le nouveau coefficient doit s'appliquer correctement.  

### EEdomus

En cours  

## Changelog

### Version 0006

* Add battery percentage remaining attribute / reporting (cluster : 0x0001 / attribute : 0x0021)
* Fix consumption leak when led is ON in sleeping mode
* Fix consumption leak when impulsion is locked (need to unsold hardware pullup)
* Fix voltage accuracy
* Fix temperature accuracy
* Disable debug console
  
### Version 0005    

* Fix debounce pulses to 20ms

### Version 0004    

* Change debounce (on pulses) management. Useful for some counters
* Send report earlier (on press instead of release)

### Version 0003  

* Add divisor coefficient to counter

### Version 0002

* Add watchdog for software timer too long
* Add stopwaketimer
* Optimisze cleaning before sleep
* Fix debouncing button
* Fix manufacturer name length
* Delete OTA Config

### Version 0001 (Initial version)

