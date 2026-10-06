# AT LoRa Dongle - Uživatelská příručka

## Co je AT LoRa Dongle?

**Ropixon AT-USB LoRa Dongle** umožňuje odesílat a přijímat veškeré LoRa pakety! Stačí ho správně nakonfigurovat za pomocí intuitivních At prikazu přes seriovou linku (napr. YAT pro windows, dev/ttyUSB0 na linuxu). Není potreba nic programovat. Je Vhodný pro rychlý vývoj, testování, nebo zhotovení vlastního bezdrátového LoRa pojítka na větší vzdálenost než umí např spojení přes WIFI, BLE, nebo jiné bezdratove modulace jako FSK či.OOK. Maximální Link Budget je 155.1 dB!

Při odesílání paketů není použito žádné další "obalování", nebo šifrování dat - vždy posíláte RAW (hex, nebo ASCI) data. Stejné je to s příjmem. 

Pro rychlý rozjezd.
1) Nainstalujte driver dle vašeho systému https://www.silabs.com/interface/usb-bridges/classic/device.cp2102?tab=softwareandtools
2) Pripojte at dongle do USB a Nastavte Pozadovanou konfiguraci LoRa parametrů (nebo nechte defaultní)
3) Odesílejte data skrze AT prikazy: 
a) Odeslání dat v ASCII: AT+RF_TX_TXT=Hello, this is AT LoRa dongle Ropixon!
b) Odeslání dat v HEX: AT+RF_TX_HEX=48656C6C6F2C2074686973206973204154204C6F526120646F6E676C6520526F7069786F6E21
4) Prijem je aktivovany automaticky - lze zvolit výstupní formát HEX/ASCII za pomocí "AT+RF_RX_FORMAT=HEX|ASCII, ?"

Jak se orientovat v konfugirvání vsech parametrů? zde je online calculator, kde je jasne videt co se deje s daty s ruznym modulacnim nanstavenim (jako device zvolte SX1262) https://www.semtech.com/design-support/lora-calculator nebo offline verze stažitelná ze stránky výrobce Semtech: https://semtech.my.salesforce.com/sfc/p/E0000000JelG/a/2R000000Q2OT/GhbZe2lGVNO6sNDUlo6lcHVaKMQvcVCdaYfFeSjyitk?__hstc=212684107.9ff8b12b6e565614adc931eb9cd58715.1767815254606.1767815254606.1768633713033.2&__hssc=212684107.1.1768633713033&__hsfp=c085e67af023e15dacbc592978547f63
Platí jendoduché pravidlo:
Pro vysoké přenosové rychlosti volte vysoké BW a nízké SF - tím se však snižuje schopnost přijímače získávat data na větší vzdálenost. Naopak pro velké zhoršené přenososvé podmínky je potřeba SF navyšovat a BW snižovat. To vše ovlivnujě dobu letu pakeu (TimeOnAir TOA). TOA lze také jendnoduše ověřit pomocí příkazu: "AT+RF_GET_TOA=<packet_size_bytes>", výsledek je počítán z aktuálního nastavneí LoRy pro TX.

**Aux pins**

Zařízení má 8 aux pinů, které je možné "ručně" ovládat, nebo na nich pustit PWM modulaci: "AT+AUX_PULSE=<pin:1-8>,<freq_hz>,<duty%:0-100>". Piny 7-8 jedou na HW PWM timeru (do 320 kHz), piny 1-6 na SW timeru (do 500 Hz).

**Stavové LED**
Dongle má 3 LED, které plní tyto fce:
- RED = Příjem nových AT příkazů
- GREEN = systémová LED - značí funkční chod celéo zařízení
- BLUE = Příjem nebo odeslání LoRa dat

### Hlavní vlastnosti:
- **RF LoRa Čip**: SX1262 LoRa transceiver from Semtech - Datasheet zde: https://www.semtech.com/products/wireless-rf/lora-connect/sx1262
- **Rozhraní**: USB (Virtual COM port) - Drivers can be found here: https://www.silabs.com/interface/usb-bridges/classic/device.cp2102?tab=softwareandtools
- **Ovládání**: AT příkazy přes seriovou linku
- **Podporované baud rate**: 9600, 19200, 38400, 57600, 115200, 230400 (výchozí/doporučeno)
- **Frekvence**: 150 MHz - 960 MHz (typicky 863-870 MHz pro EU, 915 MHZ pro USA), RF cesta je přizpůsobena na pásma 868 MHz, ale i na 915 MHz není problém dongle provozovat. Čím dále budeme od pracovního pásma, tím menší výkon se podaří z dongelu vyzářit/přijmout.
- **TX výkon**: až 22 dBm
- **Spreading Factor**: SF5 až SF12
- **Bandwidths**: 7.81 kHz až 500 kHz

### 1. K čemu slouží?

AT LoRa Dongle je vhodný pro:
- **Testování a vývoj** LoRa aplikací
- **Point-to-Point komunikaci** na velké vzdálenosti
- **Range testy** - měření dosahu LoRa spojení
- **Senzorové sítě** - sběr dat ze vzdálených sensorů
- **RF testing** - testování RF parametrů, CW vysílání
- **Scanning** - skenování a chytání "náhodných" paketů které se éterem šíří.

### 2. Konfigurace připojení sériového temrinálu:
- **Baud rate**: 230400 (výchozí)
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1
- **Flow control**: None
- **Line ending**: CR+LF (\r\n)

### 3. Test komunikace
Pošlete příkaz:
```
AT (nebo AT+HELP)
```
Dongle odpoví výpisem všech dostupných příkazů.
```
AT+IDENTIFY
```
Dongle odpoví "Ropixon AT-USB LoRa Dongle, FW: v1.1.0, HW: <freq_band> <osc_type> v1.0, Build: <datum> <čas>, UID: xxxxxxxxxxxxxxxxxxxxxxxx"

---

## AT Příkazy - Kompletní přehled

### Základní systémové příkazy

| Příkaz | Popis | Příklad |
|--------|-------|---------|
| `AT` | Vypíše help | `AT` |
| `AT+HELP` | Zobrazí všechny dostupné příkazy | `AT+HELP` |
| `AT+IDENTIFY` | Vrátí unique device ID | `AT+IDENTIFY` |
| `AT+CAPS` | Vypíše schopnosti firmware | `AT+CAPS` |
| `AT+FACTORY_RST` | Reset na tovární veškerého nastavení a následný restart | `AT+FACTORY_RST` |
| `AT+SYS_RESTART` | Restart systému | `AT+SYS_RESTART` |
| `AT+UART_BAUD` | Nastavení/dotaz baud rate, při nastavení se provede restart | `AT+UART_BAUD=230400` |

**Schopnosti firmware (`AT+CAPS`)** - jeden řádek, tokeny oddělené čárkou, bez mezer, bez `OK` na konci:
```
+CAPS:HOST_V1,TX_DONE,RX_TO_UART,RX_FORMAT,TX_SAVED,TX_CW,TOA,AUX,AUX_PWM_HW,UART_BAUD
```
`HOST_V1` je vždy přítomen a označuje podporu host protokolu v1 (vyžaduje ho Python driver). Host aplikace má hledat jednotlivé tokeny, ne porovnávat celý řádek.

### LoRa TX parametry (vysílání)

| Příkaz | Popis | Rozsah hodnot | Příklad |
|--------|-------|---------------|---------|
| `AT+LR_TX_FREQ` | TX frekvence v Hz | - | `AT+LR_TX_FREQ=869525000` |
| `AT+LR_TX_POWER` | TX výkon | -17 až 22 dBm | `AT+LR_TX_POWER=14` |
| `AT+LR_TX_SF` | Spreading Factor | 5 až 12 | `AT+LR_TX_SF=7` |
| `AT+LR_TX_BW` | Bandwidth index | 0-9 (viz tabulka níže) | `AT+LR_TX_BW=7` |
| `AT+LR_TX_CR` | Coding Rate | 45, 46, 47, 48 (4/5 až 4/8) | `AT+LR_TX_CR=45` |
| `AT+LR_TX_IQ_INV` | IQ inverze | 0=vypnuto, 1=zapnuto | `AT+LR_TX_IQ_INV=0` |
| `AT+LR_TX_HEADERMODE` | Header mód | 0=explicit, 1=implicit | `AT+LR_TX_HEADERMODE=0` |
| `AT+LR_TX_CRC` | CRC kontrola | 0=vypnuto, 1=zapnuto | `AT+LR_TX_CRC=1` |
| `AT+LR_TX_PREAMBLE_SIZE` | Délka preambule | 1-65535 (opt. ≥8) | `AT+LR_TX_PREAMBLE_SIZE=8` |
| `AT+LR_TX_LDRO` | Low Data Rate Opt. | 0=off, 1=on, 2=auto | `AT+LR_TX_LDRO=2` |
| `AT+LR_TX_SYNCWORD` | Sync word | 0x12 nebo 0x34 | `AT+LR_TX_SYNCWORD=0x12` |

### LoRa RX parametry (příjem)

| Příkaz | Popis | Rozsah hodnot | Příklad |
|--------|-------|---------------|---------|
| `AT+LR_RX_FREQ` | RX frekvence v Hz | - | `AT+LR_RX_FREQ=869525000` |
| `AT+LR_RX_SF` | Spreading Factor | 5 až 12 | `AT+LR_RX_SF=7` |
| `AT+LR_RX_BW` | Bandwidth index | 0-9 (viz tabulka níže) | `AT+LR_RX_BW=7` |
| `AT+LR_RX_CR` | Coding Rate | 45, 46, 47, 48 | `AT+LR_RX_CR=45` |
| `AT+LR_RX_IQ_INV` | IQ inverze | 0=vypnuto, 1=zapnuto | `AT+LR_RX_IQ_INV=0` |
| `AT+LR_RX_HEADERMODE` | Header mód | 0=explicit, 1=implicit | `AT+LR_RX_HEADERMODE=0` |
| `AT+LR_RX_CRC` | CRC kontrola | 0=vypnuto, 1=zapnuto | `AT+LR_RX_CRC=1` |
| `AT+LR_RX_PREAMBLE_SIZE` | Délka preambule | 1-65535 (≥TX) | `AT+LR_RX_PREAMBLE_SIZE=8` |
| `AT+LR_RX_LDRO` | Low Data Rate Opt. | 0=off, 1=on, 2=auto | `AT+LR_RX_LDRO=2` |
| `AT+LR_RX_SYNCWORD` | Sync word | 0x12 nebo 0x34 | `AT+LR_RX_SYNCWORD=0x12` |
| `AT+LR_RX_PLDLEN` | Délka payloadu (implicit) | 1-255 | `AT+LR_RX_PLDLEN=10` |

**Bandwidth indexy:**
| Index | Bandwidth |
|-------|-----------|
| 0 | 7.81 kHz |
| 1 | 10.42 kHz |
| 2 | 15.63 kHz |
| 3 | 20.83 kHz |
| 4 | 31.25 kHz |
| 5 | 41.67 kHz |
| 6 | 62.5 kHz |
| 7 | 125 kHz |
| 8 | 250 kHz |
| 9 | 500 kHz |

### Rychlé nastavení (všechny RF parametry najednou)

**TX konfigurace:**
```
AT+LR_TX_SET=SF:7,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22
```

**RX konfigurace:**
```
AT+LR_RX_SET=SF:7,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
```

### RF vysílání a příjem

| Příkaz | Popis | Příklad |
|--------|-------|---------|
| `AT+RF_TX_HEX` | Vyslat data v HEX formátu | `AT+RF_TX_HEX=48656C6C6F` |
| `AT+RF_TX_TXT` | Vyslat data jako text | `AT+RF_TX_TXT=Hello` |
| `AT+RF_RX_TO_UART` | Příjem na UART zapnout/vypnout | `AT+RF_RX_TO_UART=1` |
| `AT+RF_RX_FORMAT` | Formát výstupu RX dat | `AT+RF_RX_FORMAT=HEX` |

**Přijatá data se zobrazí automaticky** (pokud je povoleno RX_TO_UART=1 (ON), =0 (OFF), ?):
Formát výpisu přijatých dat
```
+RX:5,48656C6C6F,RSSI:-45
```
Kde 5 = pocet prijatých bajtu, 48656C6C6F = samotný paket, -45 = RSSI paketu

**Dokončení vysílání** se oznámí automaticky:
```
TX:DONE
```
Vypíše se po každém dokončeném RF vysílání (`AT+RF_TX_HEX`, `AT+RF_TX_TXT`, `AT+RF_TX_SAVED` a každý paket z `AT+RF_TX_SAVED_REPEAT`).

### Uložené pakety a periodické vysílání
Do AT dongelu je možné uložit jeden paket (NVMA pamět), ten je pak možné kdykoliv odeslat, nebo nastavit periodické odesílání
| Příkaz | Popis | Příklad |
|--------|-------|---------|
| `AT+RF_SAVE_PACKET` | Uložit paket do paměti | `AT+RF_SAVE_PACKET=010203` |
| `AT+RF_TX_SAVED` | Vyslat uložený paket 1x | `AT+RF_TX_SAVED` |
| `AT+RF_TX_SAVED_REPEAT` | Start/stop periodického TX | `AT+RF_TX_SAVED_REPEAT=ON` |
| `AT+RF_TX_NVM_PERIOD` | Nastavit periodu vysílání uloženého paketu | `AT+RF_TX_NVM_PERIOD=1000` |
| `AT+RF_TX_PERIOD_STATUS` | Aktuální status periodického TX | `AT+RF_TX_PERIOD_STATUS?` |

### RF test příkazy
Pro různé testovací účely se mohou hodit tyto 3 příkazi, obzvláště "AT+RF_TX_CW=1 (ON), =0 (OFF), ?", která zapne vysílání nosné frekvence (žádná modulovaná data).

| Příkaz | Popis | Příklad |
|--------|-------|---------|
| `AT+RF_TX_CW` | Zapnout/vypnout CW (continous wave) vysílání | `AT+RF_TX_CW=1` |
| `AT+RF_GET_TOA` | Spočítat Time on Air z aktuáního nastavení pro TX, parametr je délka paketu v B | `AT+RF_GET_TOA=10` |
| `AT+RF_GET_TSYM` | Získat čas jendoho symbolu z aktuálního nastavení pro TX| `AT+RF_GET_TSYM` |

### AUX GPIO piny (1-8)

| Příkaz | Popis | Příklad |
|--------|-------|---------|
| `AT+AUX` | Nastavit stav pinu | `AT+AUX=1,1` (pin 1 HIGH) |
| `AT+AUX_PULSE` | Start PWM | `AT+AUX_PULSE=1,1,50` (pin 1, 1Hz, duty 50%) |
| `AT+AUX_PULSE_STOP` | Stop PWM | `AT+AUX_PULSE_STOP=1` (stop PWM on  pin 1) |

---

## Praktické příklady použití

### Příklad 1: Jednoduchá komunikace Point-to-Point

**Dongle 1 (vysílač):**
```
AT+LR_TX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,Power:22,LDRO:2
AT+RF_TX_HEX=AABBCCDD
```

**Dongle 2 (přijímač):**
```
AT+LR_RX_SET=SF:9,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
```
→ Přijímač vypíše: `+RX:4,AABBCCDD,RSSI:-47

### Příklad 2: Range test s periodickým vysíláním

**TX Dongle:**
```
AT+LR_TX_SET=SF:12,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22
AT+RF_SAVE_PACKET=31313232333334343535
AT+RF_TX_NVM_PERIOD=2000
AT+RF_TX_SAVED_REPEAT=ON
```

**RX Dongle:**
```
AT+LR_RX_SET=SF:12,BW:7,CR:45,Freq:869525000,IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2
AT+RF_RX_TO_UART=1
AT+RF_RX_FORMAT=ASCII
```
Následný příjem na RX:
+RF:5,1122334455,RSSI:-68

### Příklad 3: CW test na frekvenci

```
AT+LR_TX_FREQ=868000000
AT+LR_TX_POWER=10
AT+RF_TX_CW=1
```
→ Vysílá čistou nosnou na 868 MHz s výkonem 10 dBm

Pro vypnutí:
```
AT+RF_TX_CW=0
```

### Příklad 4: Změna baud rate

**Aktuální hodnota:**
```
AT+UART_BAUD?
→ 115200
```

**Změna na rychlejší:**
```
AT+UART_BAUD=230400
OK
```
Dongle se restartuje a přejde na 230400 baud. Terminál musíte také přepnout.

---

## Důležité poznámky

### Kompatibilita TX/RX parametrů
**Pro úspěšnou komunikaci musí být RF nastavení shodné:**
- Frekvence (TX_FREQ = RX_FREQ)
- Spreading Factor (TX_SF = RX_SF)
- Bandwidth (TX_BW = RX_BW)
- Coding Rate (TX_CR = RX_CR)
- Sync Word (TX_SYNCWORD = RX_SYNCWORD)
- Header Mode (TX_HEADERMODE = RX_HEADERMODE)
- CRC (TX_CRC = RX_CRC)
- LDRO (LDRO 2 = automatic mode)
- IQInv (TX_IQInv = RX_IQInv)


### Optimalizace dosahu vs. rychlosti

**Maximální dosah (nejnižší rychlost):**
Vysoké SF a nízké BW -> maximální dosah -> nejpomalejší přenosová rychlost
- SF: 12
- BW: 0 (7.81 kHz)
- CR: 48 (4/8)
- Power: 22 dBm
- → Dosah: > 10 km (otevřené prostranství)

**Vysoká rychlost (kratší dosah):**
- SF: 5
- BW: 9 (500 kHz)
- CR: 45 (4/5)
- Power: 22 dBm


### Time on Air (ToA)
Čím vyšší SF a nižší BW, tím delší doba vysílání. Použijte `AT+RF_GET_TOA` pro získání doby letu paketu.

### Implicitní vs. Explicitní header
- **Explicit (0)**: Header obsahuje délku paketu
- **Implicit (1)**: Bez headeru nutné nastavit RX_PLDLEN na přijímací straně před zahájením příjmu!

### LDRO (Low Data Rate Optimization)
- **Auto (2)**: Firmware automaticky volí podle SF a BW - **doporučeno**
- **On (1)**: Manuálně zapnuto - můžete experimentovat a vybrat vhodnou variantu k vašemu dalšímu nastavení
- **Off (0)**: Manuálně vypnuto

---

## Řešení problémů

### Dongle neodpovídá na AT příkazy
1. Zkontrolujte baud rate (zkuste 115200 nebo 230400)
2. Zkontrolujte propojovací Jumpery na seriove lince - musí být propojeno MCU-USB na RX i TX headeru
3. Zkontrolujte line ending (musí být CR+LF)
4. Restartujte dongle (odpojte a připojte USB)
5. Zkuste `AT+SYS_RESTART`

### RX nevidí data z TX
1. Zkontrolujte, že všechny RF parametry pro TX i RX jsou shodné
2. Ověřte, že RX má zapnuto `AT+RF_RX_TO_UART=1`
3. Zvyšte TX výkon nebo zkraťte vzdálenost

### Nízké RSSI nebo ztráty paketů
1. Zvyšte TX výkon (max 22 dBm)
2. Snižte vzdálenost
3. Zkuste vyšší SF (7→9→12)
4. Zkuste připojit externí anténu - nutno přeletovat propojku (nulový rezistor v pouzdře 0402) z chipové antény na UFL konektor!!

### CRC chyby
1. RF interference - změňte frekvenci
2. Neshoda parametrů TX/RX
3. Zpaněte LDRO na TX i RX traně na "auto"

---

## Technické specifikace

### Hardware
- **MCU**: STM32L071 (ARM Cortex-M0+)
- **RF čip**: Semtech SX1262
- **Frekvence**: 150 MHz - 960 MHz 
- **TX výkon**: 0 až +22 dBm
- **RX citlivost**: až -149.1 dBm (SF12, BW 7.81 kHz)
- **USB**: Virtual COM port
- **GPIO**: 8x AUX pinů (PWM capable)

### Provozní podmínky
- **Napájení**: 5V z USB
- **Teplota**: -20°C až +70°C (provozní)

---

## Dodatek: Python automatizace

Pro automatizované testy a aplikace můžete použít Python s `pyserial`:

```python
#!/usr/bin/env python3

import serial
import time

PORT = "COM3"          # Windows: COM3, Linux: /dev/ttyUSB0
BAUD = 230400

FREQ = 869525000
DATA_HEX = "48656C6C6F"   # Hello
PACKET_COUNT = 5
PACKET_DELAY = 1.0

def send_AT(cmd, timeout=2.0):
    ser.reset_input_buffer()
    ser.write((cmd + "\r\n").encode())

    t0 = time.time()
    resp = ""

    while time.time() - t0 < timeout:
        if ser.in_waiting:
            resp += ser.read(ser.in_waiting).decode(errors="ignore")
            if "OK" in resp or "ERROR" in resp:
                break
        time.sleep(0.01)

    return resp.strip()


print("Open serial port")
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(0.5)

print("Identify dongle")
resp = send_AT("AT+IDENTIFY")

if "Ropixon AT-USB LoRa_Dongle" not in resp:
    print("Unknown device")
    print(resp)
    ser.close()
    exit(1)

print(resp)

print("Configure TX")
send_AT(
    f"AT+LR_TX_SET=SF:7,BW:7,CR:45,Freq:{FREQ},"
    f"IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2,Power:22"
)

print("Configure RX")
send_AT(
    f"AT+LR_RX_SET=SF:7,BW:7,CR:45,Freq:{FREQ},"
    f"IQInv:0,HeaderMode:0,CRC:1,Preamble:8,LDRO:2"
)

print("Send packets")
for i in range(PACKET_COUNT):
    resp = send_AT(f"AT+RF_TX_HEX={DATA_HEX}", timeout=3.0)
    print("Packet", i + 1, "OK" if "OK" in resp else "FAIL")
    time.sleep(PACKET_DELAY)

ser.close()
print("Done")

```


**© 2024-2026 Ropixon - AT LoRa Dongle**
