// +++++++++++++++++++++++++
//  NetTempHumSensORly (oO)
// +++++++++++++++++++++++++

// Features ------------------------------------------------------------------

#define SERIAL_MODE
#define NETWORK_MODE
// #define VERBOSE

// Macros --------------------------------------------------------------------

#ifdef SERIAL_MODE
    #define USE_SERIAL
    #ifdef VERBOSE
        #define VERBOSE_AND_SERIAL
    #endif
#endif

#ifdef VERBOSE_AND_SERIAL
    #define LOG(s)  Serial.println(s)
    #define LOGA(s) Serial.print(s)
#else
    #define LOG(s)
    #define LOGA(s)
#endif

#ifdef NETWORK_MODE
    #define USE_NETWORK
#endif

// Application ---------------------------------------------------------------

#include <dht.h>;
#ifdef USE_NETWORK
    #include <UIPEthernet.h>
#endif

// Sensor
#define DHT22_PIN  3
#define SENSOR_COOLDOWN 4000

// Network
#define CONNECTION_DELAY 200

// Board
#define LED_PIN   13
#define BAUD_RATE 9600

dht DHT;

#ifdef USE_NETWORK
    byte mac[] = { 0x42, 0x53, 0x42, 0x02, 0x00, 0x01 };
    IPAddress ip(192, 168, 178, 201);
    EthernetServer server(80);
#endif

struct
{
    uint32_t total;
    uint32_t ok;
    uint32_t crc_error;
    uint32_t time_out;
    uint32_t connect;
    uint32_t ack_l;
    uint32_t ack_h;
    uint32_t unknown;
} stat = { 0, 0, 0, 0, 0, 0, 0, 0};

#define ULONG_MAX pow(2, 32) -1
unsigned long time;
unsigned long last_sensor_read;

// Setup ---------------------------------------------------------------------

void setup() {
    setup_led();
    setup_serial();
    setup_network();
    setup_sensor();
    LOG(F("Setup finished [OK]"));
}

void setup_led() {
    pinMode(LED_PIN, OUTPUT);
}

void setup_serial() {
    #ifdef USE_SERIAL
    Serial.begin(BAUD_RATE);
    while (!Serial) ;
    LOG(F("Serial [OK]"));
    #endif
}

void setup_network() {
    #ifdef USE_NETWORK
    Ethernet.begin(mac, ip);
    server.begin();

    LOG(F("Network [OK]"));
    LOGA(F("IP Address: "));
    LOG(Ethernet.localIP());
    #endif
}

void setup_sensor() {
    LOGA(F("Sensor DHTlib "));
    LOGA(F(DHT_LIB_VERSION));
    LOGA(F(" [OK]"));

    time = millis();
    last_sensor_read = 0;
}

// Loop ----------------------------------------------------------------------

void loop() {

    time = millis();

    LOGA(F("Time: ")); LOG(time);
    LOGA(F("Last read: ")); LOG(last_sensor_read);

    bool do_read_sensor =
        ((time <  last_sensor_read) && ((ULONG_MAX - last_sensor_read + time) > SENSOR_COOLDOWN)) ||
        ((time >= last_sensor_read) && ((time - last_sensor_read) > SENSOR_COOLDOWN));

    if(do_read_sensor) {
        read_sensor();
        blink(50);
        last_sensor_read = millis();
    }

    serve_web_request();
    server_serial_request();
    blink(50);
}

// Networking ----------------------------------------------------------------

void serve_web_request() {
    #ifdef USE_NETWORK
    LOG(F("Web: Waiting for connection"));

    EthernetClient client = server.available();

    if (client)
    {
        LOG(F("Web: New Connection"));

        boolean currentLineIsBlank = true; // an http request ends with a blank line
        int headerPos = 0;
        boolean isGetRequest = false;
        char pathFirstChar = '/';
        boolean endOfHeader = false;

        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();

                if (headerPos == 0 && c == 'G') {
                    isGetRequest = true;
                } else if (isGetRequest && headerPos == 5) {
                    pathFirstChar = c;
                }
                headerPos++;

                endOfHeader = (c == '\n' && currentLineIsBlank);

                #ifdef VERBOSE_AND_SERIAL
                if (headerPos < 10) {
                    Serial.print("# ");
                    Serial.print(headerPos);
                    Serial.print(": '");
                    Serial.print(c);
                    Serial.println("'");
                }
                #endif

                if (endOfHeader) // http hackery ... first empty line
                {
                    if (pathFirstChar == 'c') {
                        client.println(F("HTTP/1.1 200 OK"));
                        client.println(F("Content-Type: text/plain; charset=UTF-8"));
                        client.println(F(""));
                        client.print(F("T,"));
                        client.print(DHT.temperature, 1);
                        client.print(F(",H,"));
                        client.println(DHT.humidity, 1);

                    } else if (pathFirstChar == 'j') {
                        client.println(F("HTTP/1.1 200 OK"));
                        client.println(F("Content-Type: application/json; charset=UTF-8"));
                        client.println(F(""));
                        client.print(F("{temperature: "));
                        client.print(DHT.temperature, 1);
                        client.print(F(",humidity: "));
                        client.print(DHT.humidity, 1);
                        client.print(F("}"));

                    } else {
                        client.println("<html><meta http-equiv=\"refresh\" content=\"60\"><title>Sensor Data</title><body></body>");

                        client.print("<div><span>Temperatur:</span> <span id=\"temperature\">");
                        client.print(DHT.temperature, 1);
                        client.print("</span><span>&deg;C</span></div>");

                        client.print("<div><span>Luftfeuchtigkeit:</span> <span id=\"humidity\">");
                        client.print(DHT.humidity, 1);
                        client.print("<span>%</span></div>");

                        client.println("</body></html>");
                    }
                    break;
                }

                if (c == '\n') {
                    currentLineIsBlank = true;
                }
                else if (c != '\r')
                {
                    currentLineIsBlank = false;
                }
            }
        }

        delay(10); // give the web browser time to receive the data
        client.stop(); // close the connection:

        LOG(F("Web: Disconnected\n"));

    } else {
        LOG(F("Web: No request"));
        delay(CONNECTION_DELAY);
    }
    #endif
}

// Sensor --------------------------------------------------------------------

int auto_write_sensor = 0;
void read_sensor() {

    uint32_t dift = micros();
    int chk = DHT.read22(DHT22_PIN);
    dift = micros() - dift;

    stat.total++;
    switch (chk) {
        case DHTLIB_OK:
            stat.ok++;
            #ifdef VERBOSE
            LOG(F("DHT22: OK,\t"));
            #endif
            break;
        case DHTLIB_ERROR_CHECKSUM:
            stat.crc_error++;
            LOG(F("DHT22: Checksum error,\t"));
            break;
        case DHTLIB_ERROR_TIMEOUT:
            stat.time_out++;
            LOG(F("DHT22: Time out error,\t"));
            break;
        case DHTLIB_ERROR_CONNECT:
            stat.connect++;
            LOG(F("DHT22: DHT22 Connect error,\t"));
            break;
        case DHTLIB_ERROR_ACK_L:
            stat.ack_l++;
            LOG(F("DHT22: Ack Low error,\t"));
            break;
        case DHTLIB_ERROR_ACK_H:
            stat.ack_h++;
            LOG(F("DHT22: Ack High error,\t"));
            break;
        default:
            stat.unknown++;
            LOG(F("DHT22: Unknown error,\t"));
            break;
    }

    // DISPLAY DATA
    #ifdef VERBOSE_AND_SERIAL
    Serial.print(F("# "));
    Serial.print(DHT.humidity, 1);
    Serial.print(F("%,\t"));
    Serial.print(DHT.temperature, 1);
    Serial.print(F("°C,\t"));
    Serial.print(dift);
    Serial.println();

    if (stat.total % 20 == 0)
    {
        Serial.print(F("# "));
        Serial.println(F("\nTOT\tOK\tCRC\tTO\tUNK"));
        Serial.print(stat.total);
        Serial.print(F("\t"));
        Serial.print(stat.ok);
        Serial.print(F("\t"));
        Serial.print(stat.crc_error);
        Serial.print(F("\t"));
        Serial.print(stat.time_out);
        Serial.print(F("\t"));
        Serial.print(stat.connect);
        Serial.print(F("\t"));
        Serial.print(stat.ack_l);
        Serial.print(F("\t"));
        Serial.print(stat.ack_h);
        Serial.print(F("\t"));
        Serial.print(stat.unknown);
        Serial.println(F("\n"));
    }
    #endif
}

void server_serial_request() {
    #ifdef SERIAL_MODE
    int serial_data = 0;
    if (Serial.available() > 0) {
        serial_data = Serial.read();

        if (serial_data == 'r') {
            write_sensor_to_serial();
        } else if (serial_data == 'a') {
            auto_write_sensor = 1;
        } else if (serial_data == 's') {
            auto_write_sensor = 0;
        }
    } else if (auto_write_sensor) {
        write_sensor_to_serial();
    }
    #endif
}

void write_sensor_to_serial() {
    #ifdef USE_SERIAL
    Serial.print(F("T,"));
    Serial.print(DHT.temperature, 1);
    Serial.print(F(",H,"));
    Serial.print(DHT.humidity, 1);
    Serial.println();
    #endif
}

// Helpers -------------------------------------------------------------------

void blink(int delayMs) {
    digitalWrite(LED_PIN, HIGH); delay(delayMs);
    digitalWrite(LED_PIN, LOW);  delay(delayMs);
}
