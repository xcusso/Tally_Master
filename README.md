Tally_Master

Codi i hardware per crear un sistema inhalambric de tally.
Utilitzant Esp32 i protocol Espnow
Basat en el projecte: https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/

Objectiu: 
Crear tallys (pilots vermells ON AIR) inhalambrics multiproposit.

Es defineixen dos tipus de dispositius: Master i Slave

Dispositiu Master: va connectat als GPIO dels equips i s'encarrega de connectar amb els slave de forma inhalambrica.
Dispositiu Slave: estan enllaçats inhalambricament amb el master. 

Tant el modul Master com Slave es poden configurar com a TALLY, CONDUCTOR o PRODUCTOR.

Hardware:
Es basa en ESP32 DEV Kit utilitzant el protocol de comunicació de la casa Espressif ESPNow per connectar els diferents dispositius.
Aquest protocol permet una ràpida connexió utilitzant la banda lliure de 2,4GHz amb un abast superior al WIFI convencional.

HARDWARE BASE:
Carcassa de plàstic: Capsa de plàstic impressa en impressora 3D.
Diseny funcional: https://www.tinkercad.com/things/bXiD0YwMSFD

Placa d'alimentació amb dos bateries del tipus 18650 que suministren 3,3V i 5V i fan funcions de càrregà i protecció d'aquestes. Es careguen via conector USB.
https://www.aliexpress.com/snapshot/0.html?spm=a2g0o.9042647.0.0.6d3763c0rcxVbg&orderId=8145150705073892&productId=1005001854607900

Atenció: Les plaques rebudes no subministren 3V, donen 1,8V a la sortida marcada com a 3V. La sortida marcada a 5V si que treu 5V. Les bateries es carreguen i descareguen en paral.lel. No fa balanç de càrrega, per tant és important tenir-les ben aparellades!
Cal investigar el commutador hold i normal per veure com afecta a la alimentació. El polsador engega i para el subminsitre de tensió. Es poden utilitzar mentre es carreguen.


Interruptor d'encessa/apagat (no utilitzat al final)
https://es.aliexpress.com/item/1005001513148147.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Placa ESp32:
https://es.aliexpress.com/item/32864722159.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Display OLED blanc: (No utilitzat al final)
https://es.aliexpress.com/item/32672229793.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Display LCD 16x2 amb interface I2C:
https://es.aliexpress.com/item/1005002186334832.html


Polsadors vermell i verd:
https://es.aliexpress.com/item/1005003120415869.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Polsadors

    3
    2
A   1   B

Vist des dels connectors:
1- Contact  
2- NO
3- NC
A- + Led
B- - Led

Pilot Led local 8x1:
https://es.aliexpress.com/item/4000195919675.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Matriu led 8x8 indicadora: (no utilitzat al final)
https://es.aliexpress.com/item/4001296811800.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.242c63c0Rw7HHY

Level shifter (per adaptar 3,3v a 5V i inversa):
https://es.aliexpress.com/item/1005001839292815.html?gatewayAdapt=glo2esp&spm=a2g0o.9042311.0.0.20cd63c0iFG4tf
En principi no caldrà al utilitzar els expansors PC8575

Modul expansor GPIO PCF8575:
https://es.aliexpress.com/item/4000981520132.html
Fa faltar PULL UP externs. Hem posat pull-ups per resistencia Arry 10K a les entrades del modul que va amb el VIA. En principi la taula QL genera els nivells de forma correcta (caldrà provar-ho).

MASTER:
El controlador master serà l'encarregat d'intercactuar mitjançant GPIO d'entrada i sortida amb senyals externes provinents de la taula de só local i dels estudis centrals, i al mateix temps enviar GPO per activar funcions de la taula local o senyalitzacions als estudis centrals.  
Fisicament serà identic als altres però inclourà dos connectors SUB D 15 de comunicació per als GPIO i tindrà accés mitjançant WIFI a internet per poder-lo configurar i adquirir parametres com el servidor de temps NTP.

Utilitzant el level shifter podem generar un bit per determinar si tenim connectat el connector GPIO de la taula QL o del VIA.

En el cas dels equips VIA, tan sols tenen quatre GPIO i el seu funcionament és una mica diferent al standart.
Sortides: El via subministra 4 contactes que internament obre i tanca com si fossin reles (relé d'estat sólid).
Les entrades detecta mitjançant contacte a terra o circuit obert. Cal verificar si es poden utilitzar directament les plaques PCF8575

De moment sense us (S'han col.locat el transistor estandart 2N2222. La sortida del ESp32 passa a una resistencia de 1k i va a la base del transistor. 
L'emissor està conectat a terra i el colector al pin del VIA.)

Utilitzarem el OUTPUT 1 del VIA per donar la senyal ON AIR. S'ha inclós el 
El bit del OUTPUT 1 vindrà del control central, el OUTPUT 4 ha d'estar a terra OFF. D'aquesta manera (al estar connectat en PULL UP) sabrem si tenim el VIA connectat o no.


Cada equip té un identificador: 
0 Master
1 Slave 1
2 Slave 2
3...

Cada equip pot configurar-se per actura com a:
0 - Debug
1 - Conductor
2 - Productor
3 - Sense efecte (només llum)

CONDUCTOR: Botó vermell dona ordres a productor local, botó verd dona ordres a estudis centrals.

PRODUCTOR: Botó vermell dona ordres a conductor, botó verd dona ordres a estudis centrals.

Codis de color:

Si només tenim connectat el Equip A (VIA) (rebem un zero en el BIT(3)4):
Si rebem el BIT0 encendrem el matrix en VERMELL si no apagat.

Si tenim només equip B (QL) connectat (detectat per la presencia de +5V del bit 6):
Si rebem el BIT0 encendrem matrix en VERD, si no apagat.

Si tenim el equip A i B connectats:

Quan es rebi GPIO ON AIR dels estudis del equip A encendre els pilots en taronja.
Quan rebi GPIO ON de la taula i també dels estudis encendre els pilots en vermell.
Quan rebi només GPIO de la taula i no dels estudis encendre en groc.

Estem limnitats als GPIO IN de la QL1 (5 max).
Bit 1 - MUTE CONDUCTOR
Bit 2 - COND Ordres Productor (vermell)
Bit 3 - COND Ordres Estudi (verd)
Bit 4 - PROD Ordres Conductor (vermell)
Bit 5 - PROD Ordres Estudi (verd)

Utilitzarem els GPIO OUT de la QL1 per confirmar les operacions.
D'aquesta manera quan s'hagi carregat l'escena coresponent enviarem el bit de confirmació als polsadors de tally. Ens queda un bit lliure d'entrada a la QL

GPIO OUT QL 1
Bit 1 - Micro CONDUCTOR ON (confirma tally)
Bit 2 - Confirmació COND Ordres Productor (led vermell polsador COND ON)
Bit 3 - Confirmació COND Ordres Estudi (led verd polsador COND ON)
Bit 4 - Confirmació PROD Ordres Conductors (led vermell polsador PROD ON)
Bit 5 - Confirmació PROD Ordres Estudi (led verd polsador PROD ON)
Bit 6 - Presencia de 5V

Que han de fer les escenes de la QL:

Quan rebem un Bit 1 - Fer MUTE del conductor a programa. 

Quan reben un Bit 2 - CONDUCTOR Ordres a Productor (vermell):
Va associat a un mute del Bit1. (Muteja el micro del conductor a PGM, PA..)
Unmute del canal CONDUCCTOR que va al BUS del PRODUCTOR. Enviar un bit pel GPIO2 (confirmacio)

Quan reben un Bit 3 - CONDUCTOR Ordres a Estudi (verd):
Va associat a un mute del Bit1. (Muteja el micro del conductor a PGM, PA..)
Fa unmute del canal del CONDUCTOR que va al BUS de ORDRES ESTUDI OUT. Envia un bit pel GPIO3 (confirmació)
Fa unmute del canal ORDRES ESTUDI IN que va al BUS del CONDUCTOR. Podria fer una atenuació del PGM del BUS del conductor.


Quan reben un Bit 4 - PRODUCTOR Ordres a Conductor (vermell):
Desmutejar micro productor del BUS que va als auriculars del conductor i del productor, (opcionalment atenuar PGM dels auriculars del conductor i del productor - pq aquest s'escolti a si mateix), enviar un bit per la sortida 4.

Quan reben un Bit 5 - PRODUCTOR Ordres Estudi (verd):
Desmutejar micro productor del BUS que va a les ordres del estudi i als auriculars del productor, (opcionalment atenuar PGM dels auriculars del productor - pq aquest s'escolti a si mateix), enviar un bit per la sortida 5.

Operativa del TALLY:

Si no rebem massa pel bit 4A (tensio) del VIA (A), missatge indicant DESCONECTAT DEL GPIO A (VIA). 
Si no rebem res pel bit 6B (tensio) de la QL (B), missatge indicant DESCONECTAT DEL GPIO B (QL). 

Si tenim la massa bit 4A del VIA (A) i no del QL (B) quan rebem el bit 1A (VIA) encendrem el tally en VERMELL. D'aquesta manera podem utilitzar el tally només amb el VIA.

Si tenim el bit 6B de la QL (B) i no del VIA (A) quan rebem el bit 1B (QL) encendrem el tally VERD (LOCAL REC). D'aquesta manera indiquem que estem gravant o enviant per un altre camí. 

Si tenim la massa del 4A de (VIA) i el 6B de (QL) - els dos equips conectats:
Si tenim el bit 1A del VIA i el bit 1B de la QL encendrem el tally en VERMELL. (ON AIR)
Si només tenim el bit 1A (VIA) i no el bit 1B (QL) encendrem el tally TARONJA. (PRECAUCIÓ ESTUDI OBERT)
Si només tenim el bit 1B (QL) i no tenim el bit 1A (VIA) encendrem el tally GROC. (ESPERANT ESTUDI).
Si no tenim cap bit 1A ni 1B el tally queda apagat.

GPIO del VIA: Connector inferior

Disposem de 5 GPI i 5 GPO del VIA.

GPIO OUT VIA

Bit 1 - Canal obert
Bit 2 - Lliure
Bit 3 - LLiure
Bit 4 - lliure
Fem servir la masa del 13 perd etectactar la connexió del equip. El PCF8578 esta en PULL UP, si no hi ha el cable detectara un 1.


GPIO IN VIA
Bit 1 - Lliure Disponible per fer invents de heartbeat.
Bit 2 - Lliure
Bit 3 - lliure
Bit 4 - Lliure

Pins del D15 - CONNECTOR SUPERIOR
1 - ground
2 - Out 4 
3 - Out 3
4 - Out 2
5 - Out 1
6 - ground a GND de PCF8578
7 - Input 3
8 - Input 1
9 - Conectat a 1 ground
10 - Connectat a 1 ground
11 - Connectat a 1 ground
12 - Connetat a 1 Ground
13 - Conectat a input 5 del PCF8578 en pullip per detectar presencia del VIA.
14 - Input 4
15 - Input 2



Podriem instal.lar un heartbeat per comprovar que la linea és operativa.

Que podria fer: 
El master es connecta a internet i ofereix el temps real al display.
Cada posició pot tenir un numero indicant la posició de la taula. El numero canvi de color.

Modul Master:

8 GPI IN (QL) Utilitzats 5
8 GPIO OUT (QL) Utilitzats 5
8 GPI IN (VIA) Utilitzats 1 (2 si contem la presencia)
8 GPIO OUT (VIA) Sense us

Modul Tally:

Inputs:
Polsador Vermell
Polsador verd
Analog IN (Nivell Bateria) Cal fer un petit divisor de tensió

Outputs: 
SDA - Display
SCL - Display
Digital matrix led (Display garn i petit)
Led Vermell
Led Verd

Inputs:
2 Digital IN
1 Analog IN


Revisió de funcionament amb la QL:

Farem servir els bits 2,3,4 i 5 d'entrada a la QL per fer: 

Bit 1 - Mute COND
Bit 2 - COND Ordres Productor (vermell)
Bit 3 - COND Ordres Estudi (verd)
Bit 4 - PROD Ordres Conductors (vermell)
Bit 5 - PROD Ordres Estudi (verd)

Ordres a de Conductor a Productor:
Al apretar boto vermell del conductor:
Arriba un 1 al bit 1 (mute COND)
Arriba un 1 al bit 2 (Ordres Prod)

Quan arribi 1 a Bit 1 sempre fa un mute del COND
Quan arribi 1 a Bit 2 es fa unmute de la copia del conductor que va al mix del productor.

Ordres a de Conductor a Estudi:
Al apretar boto verd del conductor:
Arriba un 1 al bit 1 (mute COND)
Arriba un 1 al bit 3 (Ordres Prod)

Quan arribi 1 a Bit 1 sempre fa un mute del COND
Quan arribi 1 a Bit 2 es fa unmute de la copia del conductor que va al mix del ordres estudi. Es fa unmute de les ordres del estudi al conductor.

Ordres a de Productor a Estudi:
Al apretar boto verd del productor:
Arriba un 1 al bit 5 (Ordres EST) 

Fem unmute ordres Productor a Estudi.
Posem retorn atenuat (mute retorn normal)

Per implentar el NTP: https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/