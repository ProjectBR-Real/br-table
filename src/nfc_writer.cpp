#ifdef NFC_WRITER
#include <SPI.h>
#include <MFRC522.h>
#include <NfcAdapter.h>

constexpr uint8_t RST_PIN = 9;
constexpr uint8_t SS_PIN = 10;
constexpr uint8_t BUZZER_PIN = 7;

MFRC522 mfrc522(SS_PIN, RST_PIN);
NfcAdapter nfc = NfcAdapter(&mfrc522);

// 書き込むアイテムデータ
const char* ITEM_TYPES[] = {
    "ITEM:CIG",      // 0: Cigarette
    "ITEM:BEER",     // 1: Beer
    "ITEM:SAW",      // 2: Saw
    "ITEM:CUFF",     // 3: Handcuffs
    "ITEM:MAG"       // 4: MagnifyingGlass
};

const char* ITEM_NAMES[] = {
    "Cigarette",
    "Beer",
    "Saw",
    "Handcuffs",
    "MagnifyingGlass"
};

void playSuccessBeep() {
    tone(BUZZER_PIN, 523, 100);  // C5
    delay(120);
    tone(BUZZER_PIN, 659, 100);  // E5
    delay(120);
    tone(BUZZER_PIN, 784, 150);  // G5
    noTone(BUZZER_PIN);
}

void playErrorBeep() {
    tone(BUZZER_PIN, 200, 300);
    delay(350);
    tone(BUZZER_PIN, 150, 300);
    noTone(BUZZER_PIN);
}

bool writeItemToTag(const char* itemData) {
    Serial.print(F("Writing NDEF message: "));
    Serial.println(itemData);
    
    // NDEFメッセージを作成
    NdefMessage message = NdefMessage();
    message.addTextRecord(itemData);
    
    // タグに書き込み
    bool success = nfc.write(message);
    
    if (success) {
        Serial.println(F("Write successful!"));
        return true;
    } else {
        Serial.println(F("Write failed!"));
        Serial.println(F("Make sure tag is writable and properly formatted"));
        return false;
    }
}

void printMenu() {
    Serial.println(F("\n================================="));
    Serial.println(F("   NFC Tag Writer (NDEF Format)"));
    Serial.println(F("================================="));
    Serial.println(F("Select item to write:"));
    Serial.println(F("  0: Cigarette (ITEM:CIG)"));
    Serial.println(F("  1: Beer (ITEM:BEER)"));
    Serial.println(F("  2: Saw (ITEM:SAW)"));
    Serial.println(F("  3: Handcuffs (ITEM:CUFF)"));
    Serial.println(F("  4: MagnifyingGlass (ITEM:MAG)"));
    Serial.println(F("  8: Format tag as NDEF"));
    Serial.println(F("  9: Show menu again"));
    Serial.println(F("================================="));
    Serial.println(F("Tip: Format tag first (option 8) if write fails"));
    Serial.println(F("=================================\n"));
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    pinMode(SS_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    SPI.begin();
    mfrc522.PCD_Init();
    mfrc522.PCD_DumpVersionToSerial();

    Serial.println(F("Initializing NFC adapter..."));
    nfc.begin();
    Serial.println(F("\n*** NFC Tag Writer Ready ***"));
    printMenu();
}

int selectedItem = -1;
bool formatMode = false;

void loop() {
    // シリアルからの入力を処理
    if (Serial.available() > 0) {
        char input = Serial.read();
        
        if (input == '9') {
            printMenu();
            return;
        }
        
        if (input == '8') {
            formatMode = true;
            selectedItem = -1;
            Serial.println(F("\nFormat mode selected."));
            Serial.println(F("Place NFC tag on reader to format..."));
            return;
        }
        
        int itemIndex = input - '0';
        if (itemIndex >= 0 && itemIndex < 5) {
            selectedItem = itemIndex;
            formatMode = false;
            Serial.print(F("\nSelected: "));
            Serial.println(ITEM_NAMES[selectedItem]);
            Serial.println(F("Place NFC tag on reader..."));
        } else {
            Serial.println(F("Invalid selection. Press 9 for menu."));
        }
    }
    
    // フォーマットモードの場合
    if (formatMode) {
        if (nfc.tagPresent()) {
            Serial.println(F("\nTag detected! Formatting as NDEF..."));
            
            bool success = nfc.format();
            
            if (success) {
                Serial.println(F("Format successful!"));
                Serial.println(F("Tag is now ready for writing."));
                playSuccessBeep();
            } else {
                Serial.println(F("Format failed!"));
                Serial.println(F("Tag may not be compatible or is write-protected."));
                playErrorBeep();
            }
            
            formatMode = false;
            Serial.println(F("\n>> Select item to write or press 9 for menu"));
            delay(2000);
        }
    }
    // アイテムが選択されている場合、タグの検知を待つ
    else if (selectedItem >= 0) {
        if (nfc.tagPresent()) {
            Serial.println(F("\nTag detected!"));
            
            bool success = writeItemToTag(ITEM_TYPES[selectedItem]);
            
            if (success) {
                Serial.print(F("Successfully wrote: "));
                Serial.println(ITEM_NAMES[selectedItem]);
                playSuccessBeep();
            } else {
                Serial.println(F("Failed to write!"));
                Serial.println(F("Try formatting the tag first (option 8)"));
                playErrorBeep();
            }
            
            // 次のアイテム選択を待つ
            selectedItem = -1;
            Serial.println(F("\n>> Select next item or press 9 for menu"));
            
            delay(2000);
        }
    }
    
    delay(100);
}

#endif // NFC_WRITER
