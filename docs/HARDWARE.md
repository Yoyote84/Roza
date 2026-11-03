# Référence matérielle — Cardputer ADV + Cap LoRa-1262

Les broches ci-dessous proviennent des fiches M5Stack. Vérifier la révision physique avant
tout branchement externe.

## Cardputer ADV

| Fonction | GPIO |
|---|---:|
| Écran rétroéclairage / alimentation LED RGB | G38 |
| LCD RST | G33 |
| LCD RS | G34 |
| LCD MOSI | G35 |
| LCD SCK | G36 |
| LCD CS | G37 |
| I²C SDA | G8 |
| I²C SCL | G9 |
| TCA8418 INT | G11 |
| Batterie ADC | G10 |
| microSD CS | G12 |
| microSD MOSI | G14 |
| microSD CLK | G40 |
| microSD MISO | G39 |
| IR TX | G44 |
| Audio ES8311 | G8/G9 + lignes I²S dédiées |

## Cap LoRa-1262

| Fonction | GPIO |
|---|---:|
| SX1262 RESET | G3 |
| SX1262 IRQ | G4 |
| SX1262 BUSY | G6 |
| SX1262 SCK | G40 |
| SX1262 MOSI | G14 |
| SX1262 MISO | G39 |
| SX1262 NSS | G5 |
| GNSS RX hôte | G13 |
| GNSS TX hôte | G15 |
| Expandeur/I²C SDA | G8 |
| Expandeur/I²C SCL | G9 |

## Conflits apparents à traiter comme bus partagés

- microSD et SX1262 partagent SCK=G40, MOSI=G14, MISO=G39.
  Ils ont des CS distincts (SD G12, radio G5). Implémenter un gestionnaire SPI commun.
- I²C G8/G9 est partagé entre le clavier TCA8418, le BMI270 et l'expandeur du Cap.
  Implémenter un gestionnaire I²C commun et vérifier les adresses sur le matériel.
- Le GNSS utilise UART sur G13/G15.
- Avant émission radio, activer l'interrupteur d'antenne RF via P0 de l'expandeur
  PI4IOE5V6408, conformément à la procédure M5Stack.

## Avertissements

- Antenne RP-SMA installée avant mise sous tension/émission du module.
- Ne pas brancher/débrancher le Cap sous tension.
- Le module LoRa peut consommer fortement en émission ; mesurer le courant réel.
- Ne pas supposer qu'un ordre logiciel de sommeil coupe l'alimentation du module.
- Ne pas formater la microSD sans sauvegarde.
- Les cartes SDXC 128 Go sont souvent en exFAT. La prise en charge exFAT n'est pas présumée :
  identifier le format et tester le pilote choisi. Pour le premier test, utiliser une carte de
  test ou une sauvegarde intégrale, pas les données originales.
- La documentation M5Stack consultée présente des valeurs de capacité de batterie différentes
  selon les pages/révisions. Mesurer la tension et caractériser l'autonomie sur l'appareil réel.
