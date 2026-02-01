#ifndef NFC_WRITER
#include <SPI.h>
#include <MFRC522.h>
#include <NfcAdapter.h>

constexpr uint8_t RST_PIN = 9;
constexpr uint8_t SS_PIN = 10;
constexpr uint8_t BUZZER_PIN = 7;
constexpr unsigned long DUPLICATE_PREVENT_MS = 2000;  // 同一タグの連続読み込み防止時間（2秒）

MFRC522 mfrc522(SS_PIN, RST_PIN);
NfcAdapter nfc = NfcAdapter(&mfrc522);

// 前回のループでタグが存在していたか
bool lastTagPresent = false;

void playBeep() {
    tone(BUZZER_PIN, 440, 100);  // A4 (440Hz)
    delay(150);
    tone(BUZZER_PIN, 660, 100);  // E5 (660Hz)
    delay(150);
    noTone(BUZZER_PIN);
}

// 2つのUID配列を比較
bool isUIDEqual(const byte* uid1, int len1, const byte* uid2, int len2) {
    if (len1 != len2) return false;
    for (int i = 0; i < len1; i++) {
        if (uid1[i] != uid2[i]) return false;
    }
    return true;
}

void readItemFromTag() {
    // NDEFメッセージを読み取る
    NfcTag tag = nfc.read();
    
    if (tag.hasNdefMessage()) {
        NdefMessage message = tag.getNdefMessage();
        int recordCount = message.getRecordCount();
        
        for (int i = 0; i < recordCount; i++) {
            NdefRecord record = message.getRecord(i);
            
            // テキストレコードの場合 (Type == 'T')
            const byte* type = record.getType();
            const int typeLength = record.getTypeLength();
            if (typeLength == 1 && type[0] == 'T') {
                const int payloadLength = record.getPayloadLength();
                const byte* payload = record.getPayload();
                if (payloadLength <= 0 || payload == nullptr) {
                    continue;
                }
                
                // テキストレコード: payload[0]に言語コード長が入る
                const byte langLength = payload[0] & 0x3F;
                const int textStart = 1 + langLength;
                if (textStart >= payloadLength) {
                    continue;
                }
                
                String text = "";
                for (int j = textStart; j < payloadLength; j++) {
                    text += (char)payload[j];
                }
                
                // "ITEM:XXX" 形式かチェック
                if (text.startsWith("ITEM:")) {
                    Serial.println(text);
                    playBeep();
                    return;
                }
            }
        }
        
        Serial.println(F("NO_ITEM_DATA"));
    } else {
        Serial.println(F("NO_NDEF_MESSAGE"));
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    pinMode(SS_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    SPI.begin();
    mfrc522.PCD_Init();
    mfrc522.PCD_DumpVersionToSerial();

    Serial.println(F("Initializing NFC reader..."));
    nfc.begin();
    Serial.println(F("Ready to scan NFC tags (NDEF format)..."));
}

void loop() {
    if (nfc.tagPresent()) {
        // タグが新規に置かれた場合のみ読み込み
        if (!lastTagPresent) {
            readItemFromTag();
            // タグ処理後、MFRC522をハルト状態にする
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
        }
        lastTagPresent = true;
    } else {
        // タグが取り除かれた
        lastTagPresent = false;
    }
    
    delay(100);  // ポーリング間隔
}

#endif // NFC_WRITER