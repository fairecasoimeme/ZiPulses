# ZiPulses
Compteur d'impulsions communiquant en ZigBee 3.0.  
Le ZiPulses est disponible en boutique :  
[Boutique LiXee](https://lixee.fr/)

## Description

## PCB

## Installation

## Voyant lumineux

## Clusters

### Clusters List

|Num (Hexa)|Name|I/O| Comment|
|----------|----|---|--------|  
|0x0000|Basic|I||
|0x0001|PowerConfiguration|I| Doit être "bind" pour recevoir les trames|
|0x0003|Identify|I||
|0x0402|Temperature Measurement|I|Doit être "bind" pour recevoir les trames|
|0x0702|Simple Metering|I|Doit être "bind" pour recevoir les trames|
|0x0019|OTA|O|Not used yet|

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

