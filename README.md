# ZiPulses
Compteur d'impulsions communiquant en ZigBee 3.0.  
Le ZiPulses est disponible en boutique :  
[Boutique LiXee](https://lixee.fr/)

## Description

L'appareil **ZiPulses** permet de récupérer les impulsions d'un compteur à impulsions pour créer un index et le transmettre à un coordinateur Zigbee.  
**ZiPulses** est un capteur Zigbee de type "ZED" (Zigbee End Device). Il ne peut donc pas router les informations des autres capteurs et nécessite des périodes d'endormissement afin de préserver la consommation d'énergie.  
  
**ZiPulses** peut être alimenté de deux façons:  
* avec une pile CR2450 (3V)
* avec une alimentation 5V à 12V

### Fonctionnement

#### Appairage

##### Si le produit n'est associé à aucun réseau zigbee
* ZiPulses se mettra en mode appairage dès la mise sous tension. (la Led clignote 3 sec)
* ZiPulses se mettra en mode appairage à chaque appuie sur le bouton "Link". (la Led clignote 3 sec)

##### Si le produit est déjà associé à un réseau zigbee
* ZiPulses fera un "rejoin" à chaque mise sous tension.
* Pour sortir d'un réseau, il faut rester appuyer sur le bouton "Link" pendant 10 sec. Il se remettra automatiquement en mode appairage. (la Led clignote 3 sec)

#### Le capteur est appairé

##### Juste après l'appairage
* le coordinateur a environ 10 secondes pour faire un bind sur les clusters : 0x0001, 0x0402, 0x0702

##### ZiPulses n'est pas sollicité
* Le capteur va se réveiller et s'endormir toutes les 60 secondes
* Le capteur envoie toutes les 2 heures, la tension de batterie, la temperature et le dernier index

##### ZiPulses reçoit des impulsions
* Le capteur est reveillé. Il incrémente l'index et l'envoie au coordinateur

⚠️ **Pour que les requêtes du coordinateur au ZiPulses soit pris en compte, il est indispensable d'appuyer sur le bouton "Link" une fois juste après le lancement de la requête ZigBee (afin de le réveillé). Cette technique est notamment indispensable pour modifier le coefficient multiplicateur de l'index et/ou l'unité de mesure par exemple.
Avec la même technique, il est aussi possible d'interroger tous les attributs manuellement**


## PCB

Les PCB ci-dessous ne sont pas forcément contractuel. Ils risquent de légèrement être modifié en fonction des évolutions mais les fonctions seront similaires

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

#### Alimentation sur pile

Tout d'abord, il faut vérifier que le Switch alimentation (visible [ici](#face-2)) est bien positionné sur **BAT** (comme régulateur)  
Ensuite, il faut introduire une pile de type CR2450 dans l'emplacement en respectant la polarité. (+ en haut et - en bas)

### Impulsions

#### Compteur GAZPAR
NC

#### Compteur / débimètre eau (uniquement avec sortie impulsion de type ILS/ lamelles métallique)
NC

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
|0x0100 | CurrentTier1SummationDelivered|RO|Uint48| | Index calculé (nombre d'impulsions X coefficient multiplicateur)|
|0x0300 | unitOfMeasure|RW|Enum8| 0 (kWh) par defaut |Unité de mesure pour les impulsions. Pour choisir la bonne unité de measure cf [Tableau de mesures](#tableau-dunit%C3%A9-de-mesure)|
|0x0301 | Multiplier|RW|Uint24| 1 par defaut | Coefficient multiplicateur pour les impulsions|

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
### Home-asssitant
### EEdomus

## Changelog
### Version 0001 (Initial version)

