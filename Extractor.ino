/**************************************************************************
    Souliss - Fan / Extractor

    Gestión de un ventilador o extractor, mediante consigna (setpoint). 
    Con la consigna a debajo de 1 ºC el funcinamiento es en manual,
    por encima de esta consigna el funcionamiento es automático. 
    By Pep Ventura.
        
***************************************************************************/

// Configure the framework
#include "bconf/StandardArduino.h"          // Use a standard Arduino
#include "conf/ethW5100.h"                  // Ethernet through Wiznet W5100
#include "conf/Gateway.h"                   // The main node is the Gateway

// Include framework code and libraries
#include <SPI.h>
#include <EEPROM.h>
#include "Souliss.h"

// Include and Configure DHT11 SENSOR
#include "DHT.h"
#define DEADBAND      0.01 
#define DHTPIN        A0      // what pin we're connected to
#define DHTTYPE       DHT11   // DHT 11 
DHT dht(DHTPIN, DHTTYPE, 15); 

// Define the network configuration according to your router settings
uint8_t ip_address[4]  = {192, 168, 1, 70};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4]  = {192, 168, 1, 1};
#define myvNet_address  ip_address[3]       // The last byte of the IP address 

// Define the Slots
#define Ventilador   0  // T18
#define Temperatura  1  // T52
#define Consigna     3  // T62

#define Debug               Serial  //Change to Serial1 if you want to use the GPIO2 to TX
#define DebugDHT            1       //0 - None      / 1 - Show data on Serial  
#define Celsius             1       //0 - Farenheit / 1 Celsius

boolean fin;

void setup()
{   
    Initialize();
    
    // Set network parameters
    Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);
    SetAsGateway(myvNet_address);                              // Set this node as gateway for SoulissApp  
  
    // Define inputs, outputs pins and pulldown
    Set_StepRelay(Ventilador);
    Set_T52(Temperatura);
    Souliss_SetT62(memory_map, Consigna); 

    
    // Define inputs, outputs pins
    pinMode(4, INPUT);       // Interruptor Ventilador
    pinMode(7, OUTPUT);      // Rele Ventilador
    pinMode(A0, INPUT);      // dht
}

void loop()
{
    // Here we start to play
    EXECUTEFAST() {                     
        UPDATEFAST();   

        // Execute the code every 1 time_base_fast      
        FAST_30ms() {
        
            // Use pin4 as command
            DigIn2State (4, Souliss_T1n_OnCmd, Souliss_T1n_OffCmd,Ventilador);  // Use the pin4 as ON command
            Logic_SimpleLight(Ventilador);                                      // Drive the fan as per command
            DigOut(7, Souliss_T1n_Coil, Ventilador);                            // Use the pin7 to give power to the fan according to the logic   
              
        }

        // Maniobra de consigna para el Ventilador
        FAST_2110ms()
        {
           Logic_T52(Temperatura);
           Souliss_Logic_T62(memory_map, Consigna, 0.015, &data_changed);

            float t = dht.readTemperature();
            if ((mOutputAsFloat(Consigna) < 1)) DigOut(7, Souliss_T1n_Coil, Ventilador); 
            else if (mOutputAsFloat(Consigna) > (t))  mInput(Ventilador) = Souliss_T1n_OnCmd;
            else if (mOutputAsFloat(Consigna) <= (t))  mInput(Ventilador) = Souliss_T1n_OffCmd;
                 }
          
             FAST_GatewayComms();  
               
    }

    EXECUTESLOW() { 
        UPDATESLOW();

        SLOW_10s() {               
            // Reading temperature 
            float t = dht.readTemperature();

              Souliss_ImportAnalog(memory_map, Temperatura, &t);
        }
      }
    }
