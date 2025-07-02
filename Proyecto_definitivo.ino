#include <DHT.h>//Humedad, temperatura e índice de calor
#include <Keypad.h>
#include <LiquidCrystal.h>

//Pin al que se conecta el sensor
#define DHTPIN 8
//Seleccionar tipo de sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);
unsigned long tiempodht=0;

//Varibales Bombas y motor
int aguaAbono=9;
int residuVent=10;
int agua=30;
int abono=31;
int aguaResi=32;
int vent=33;
int shsuelo=A1;//Sensor Humedad Suelo
bool abonado=LOW;
int nivel1 = A2;
int nivel2 = A3;
int nivel3 = A4;
int tanque1 = 0;//tanque de agua
int tanque2 = 0;//tanque de abono
int tanque3 = 0;//tanque de agua residual
int setpointnivelmin = 60;
int setpointnivelmax = 65;
bool ventilador=LOW;
unsigned long calculonivel = 0;

//Variables de control al activar las bombas
int etapakm=0;//Etapa inicial
unsigned long tiempoRef=0;
unsigned long tiempoPID=0;
unsigned long tiempoError=0;
unsigned long tiempoErrorHumAmb=0;
unsigned long tiempoErrorTempAmb=0;
unsigned long tiempoLcd=0;
unsigned long tiempoAutoMan=0;
unsigned long tiempoClave=0;
unsigned long tiempoSeguridad=0;
unsigned long cuenta=0;
unsigned long tempAbono=30;//Cantidad en segundos de temporizador para el abono
unsigned long temporizadorAbono;
int retardo=1000;
int retardolcd=2000;
int retardoPID=200;

//PID
float Kp=1.5;//Multiplica por setpoint para sacar el valor del que parte (PRESENTE)
float Ki=0.6;//Más grande valor, más rápido se ajusta cuando está cerca de setpoint(PASADO)
float Kd=0.3;//Valor grande equivale a precisión pero menor suavidad y estabilidad (FUTURO)

//Variables PID
float setHumSuelo=30.0;//setpoint humedad en suelo ideal en %
float setHumAmb=80.0;//humedad máxima en ambiente que enciende ventilador
float setTempAmb=29.00;//humedad máxima en ambiente que enciende ventilador
float error=0;
float errorHumAmb=0;
float errorTempAmb=0;
float lastError=0;
float lastErrorHumAmb=0;
float lastErrorTempAmb=0;
float integral=0;
float integralHumAmb=0;
float integralTempAmb=0;
float derivative=0;
float derivativeHumAmb=0;
float derivativeTempAmb=0;
float output;
float outputHumAmb;
float outputTempAmb;
float humedad=0;

//Variables Automático y Manual
bool selector=LOW;//Low es automático, High es manual
bool unavez=LOW;//Marca interna para realizar una vez un bucle while
bool seguridad=LOW;//Marca de seguridad para poner la contraseña cada vez que se entre al modo Manual
bool conmuAntes=LOW;//Estado conmutador antes
bool conmuAhora=LOW;//Estado conmutador ahora
int conmutador=34;
bool conmutado=LOW;
int ledAuto=35;
bool automatico=LOW;
int ledMan=36;
bool manual=LOW;

//Declaración de variables globales
int led_luz=40;
int pin_luz=A0;
int luz_actual=0;
int luz=0;
float t=0;
float h=0;
char clave[5] = {'1','1','1','1','1'};

const byte filas = 4; //Número de filas
const byte cols = 4; //Número de columnas 

//Variables de control
int etapa = 0; //Etapas de la pantalla
int inf = 1; // Variable para que podamos escribir la de fomra infinita si nos equivocamos
char opcion; //Variable para saber que escribimos por pantalla en el switch
unsigned long tiempo = 0;
char eleccion;
bool entradaTemp=LOW;

LiquidCrystal lcd(7,6,5,4,3,2);

char PanelCntrl[filas][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte PinsFilas[filas] = {22,23,24,25}; //Pines de las filas
byte PinsCols[cols] = {26,27,28,29}; //Pines de las columnas

Keypad customKeypad = Keypad(makeKeymap(PanelCntrl), PinsFilas, PinsCols, filas, cols);

void setup(){
  //Setup de los motores
  pinMode(agua,OUTPUT);
  pinMode(abono,OUTPUT);
  pinMode(aguaResi,OUTPUT);
  pinMode(vent,OUTPUT);
  pinMode(aguaAbono,OUTPUT);
  pinMode(residuVent,OUTPUT);
  pinMode(nivel1,INPUT);
  pinMode(nivel2,INPUT);
  pinMode(nivel3,INPUT);
  pinMode(shsuelo,INPUT);
  digitalWrite(agua,LOW);
  digitalWrite(abono,LOW);
  digitalWrite(aguaResi,LOW);
  digitalWrite(vent,LOW);
  digitalWrite(aguaAbono,LOW);
  digitalWrite(residuVent,LOW);
  //Iniciamos el resto de elementos
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16,2);
  pinMode(led_luz, OUTPUT); 
  pinMode(pin_luz,INPUT);
  pinMode(conmutador,INPUT);
  pinMode(ledAuto,OUTPUT);
  pinMode(ledMan,OUTPUT);
}

void automan(){
  bool autoLed=LOW;
  bool manLed=LOW;
  conmuAhora=digitalRead(conmutador);
  if (conmuAhora==HIGH&&conmuAntes==LOW){
    lcd.clear();
    selector=!selector;
    automatico=LOW;
    manual=LOW;
    unavez=LOW;
    //conmutado=HIGH;
    inf=1;
    tiempoAutoMan=millis();
  }
  if (selector==LOW){
    autoLed=!autoLed;
    digitalWrite(ledAuto,autoLed);
    digitalWrite(ledMan,manLed);
    autoLed=!autoLed;
    automatico=HIGH;
  }
  if (selector==HIGH){
    manLed=!manLed;
    digitalWrite(ledAuto,autoLed);
    digitalWrite(ledMan,manLed);
    manLed=!manLed;
    manual=HIGH;
  }
  while (unavez==LOW){//Para que si pulsas varias veces solo actúe una sola vez
    if (selector==HIGH){
      lcd.setCursor(4,0);
      lcd.print("Estado: ");
      lcd.setCursor(5,1);
      lcd.print("Manual");
      if ((millis()-tiempoAutoMan)>=retardolcd){
        lcd.clear();
        conmutado=HIGH;
        unavez=HIGH;
        tiempoAutoMan=millis();
      }
    }
    if (selector==LOW){
      lcd.setCursor(4,0);
      lcd.print("Estado: ");
      lcd.setCursor(3,1);
      lcd.print("Automatico");
      if ((millis()-tiempoAutoMan)>=retardolcd){
        lcd.clear();
        conmutado=HIGH;
        unavez=HIGH;
        tiempoAutoMan=millis();
      }
    }
  }
  conmuAntes=conmuAhora;
}

void contrasena (char pass[5]){
  int cont=0; //Variable para leer cada tecla pulsada 
  int iden=0; // Variable auxiliar para saber si hemos introduciolos bien la contraseña
  int con2=5; //Variable para ajustar la posicion al escribir en la pantala
  bool etapaInicio=LOW;
  char Tecla;
  char claveUsuario[5]; //Para guardar los valores escritos
  while (inf == 1){
    if (etapaInicio==LOW){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Ingrese la clave");
    }
    automan();
    if (manual==LOW){
      inf=0;
    }
    etapaInicio=HIGH;
    Tecla = customKeypad.getKey();
    if(Tecla){
      //Comparamos el valor de cada clave pulsada, con la contraseña
      claveUsuario[cont] = Tecla;
      cont++;// sumamos a la variable para pasar al siguiente valor
      lcd.setCursor(con2,1);
      lcd.print('*');
      con2++;//actualizamos la posicionen la pantalla
      if (cont == 5){
        etapaInicio=HIGH;
        for (int i = 0; i<=4; i++){
          if (claveUsuario[i] != pass[i]){
            iden = 1;
            }
          }
        if (iden ==1){
          //si la contraseña es incorrecta iden pasa a 1 y sacamos el mesaje por pantalla
          lcd.clear();
          lcd.setCursor(5,0);
          lcd.print("CLAVE");
          lcd.setCursor(3,1);
          lcd.print("INCORRECTA");
          for (tiempoClave=0;tiempoClave<=2000;tiempoClave++){
            delay(1);
            if ((etapaInicio==HIGH)&&(tiempoClave==2000)){
              lcd.clear();
              seguridad=LOW;
              etapaInicio=LOW;
              cont=0;
              iden=0;
              con2=5;
            }
          }
        }
        else {
          lcd.clear();
          lcd.setCursor(5,0);
          lcd.print("CLAVE");
          lcd.setCursor(4,1);
          lcd.print("CORRECTA"); 
          for (tiempoClave=0;tiempoClave<=1000;tiempoClave++){
            delay(1);
            if ((etapaInicio==HIGH)&&(tiempoClave==1000)){
              lcd.clear();
              seguridad=HIGH;
              etapaInicio=LOW;
              con2 = 5;
              cont = 0;
              inf = 0;
            }
          }
        }
      }
    }
  }
}
//Funcion para comporbar en el monitor serie si cambiamos de etapas en los motores
void mostrarEtapa(){
  if ((millis()-tiempoRef)>retardo){
    Serial.print("Etapa: ");
    Serial.println(etapakm);
    tiempoRef=millis();
  }
}

//Funcion para leer la humedad y tmeperatura ambiente
void leerDHT(){
  if ((millis()-tiempodht)>=250){
    h=dht.readHumidity();
    t=dht.readTemperature();
    tiempodht=millis();
  }
}
//Funcion para leer la humedad del suelo
void leerHumedad(){//Saca un valor distinto según se llame a la función de lectura humedad suelo
  int lectura=analogRead(shsuelo);//Lee valor analógico del sensor
  humedad=map(lectura,0,1023,100,0);//Devuelve valor escalado
}

//Cotrolamos un posible error en el PID para ajustar la salida (motores)
void sacarErrorHumS(){
  if ((millis()-tiempoError)>retardoPID){
    error=setHumSuelo-humedad;//Calcular el error al setpoint
    integral+=error;//Integral es sumar cada error (siendo positivo o negativo)
    derivative=error-lastError;//Derivada es el error menos el anterior
    output=Kp*error+Ki*integral+Kd*derivative;//Fórmula de salida
    output=constrain(output,0,255);//ajustamos a rango PWM
    lastError=error;//Recuerda el error anterior
    tiempoError=millis();
  }
}
//Guarda valores de nivel de líquido de cada depósito/tanque
void nivelAgua(){
  if (((millis()-calculonivel)>=1000)){
    float altura1 = analogRead(nivel1);
    tanque1 = map(altura1,0,1023,0,100);//Ajustamos los valores recibidos del sensor
    float altura2 = analogRead(nivel2);
    tanque2 = map(altura2,0,1023,0,100);
    float altura3 = analogRead(nivel3);
    tanque3 = map(altura3,0,1023,0,100);
    calculonivel = millis();
  }
}
//Función para comprobra en el monitor serie el valor del PID del motor que este activo
void mostrarPID(){
  if ((millis()-tiempoPID)>retardo){
    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.print(" toSetHumS: ");
    Serial.print(error);
    Serial.print("  Output PID: ");
    Serial.println(output);
    Serial.println(h);
    Serial.println(t);
    Serial.print("millis: ");
    Serial.println(millis());
    Serial.print("cuenta: ");
    Serial.println(cuenta);
    Serial.print("delta: ");
    Serial.println(millis() - cuenta);
    Serial.print("abonado: ");
    Serial.println(abonado);
    Serial.println(tanque1);
    Serial.println(tanque2);
    Serial.println(tanque3);
    Serial.println(temporizadorAbono);
    tiempoPID=millis();
  }
}
//Funcion para leer la luz que recibe la planta y activar la luz artificial
int Leerdatosluz(){
  int luz_natural=analogRead(pin_luz);
  int luz_ahora=map(luz_natural,0,1023,0,255);//PWM de luz ambiental
  luz_actual=constrain(luz_ahora,0,100);
  if (luz_ahora < 30){
    digitalWrite(led_luz,HIGH);
  }
  else{
    digitalWrite(led_luz,LOW);
  }
  return luz_actual;
}
//Funcion para activar los motores si se cumplen las condiciones necesarias
void motorcitos(){
  if (output==0){
    etapakm=0;
    abonado=LOW;
    digitalWrite(agua,LOW);
    digitalWrite(abono,LOW);
    digitalWrite(aguaResi,LOW);
    digitalWrite(vent,LOW);
    digitalWrite(aguaAbono,LOW);
    digitalWrite(residuVent,LOW);
    if ((abonado==LOW)&&((millis()-cuenta)>=temporizadorAbono)){
      abonado=HIGH;
    }
    if (ventilador==HIGH){
      ventilador=LOW;
    }
  }
  if (output>0.00){
    if ((abonado==HIGH)&&(tanque2>=50)){//Tanque abono
      digitalWrite(abono,HIGH);
      analogWrite(aguaAbono,output);
      etapakm=2;
      cuenta=millis();
    }//Temporizador para deposito abono cada minuto
    if ((abonado==LOW)&&(tanque1>=55)){//Tanque agua
      digitalWrite(agua,HIGH);
      analogWrite(aguaAbono,output);
      etapakm=1;
    }
    if ((ventilador==LOW)&&(tanque3>=setpointnivelmax)&&((tanque1<setpointnivelmin)||(tanque2<setpointnivelmin))){
      digitalWrite(aguaResi,HIGH);
      analogWrite(residuVent,output);
      etapakm=3;
    }
  }
 /* if (((h>setHumAmb)||(t>setTempAmb))&&(ventilador==LOW)){
    digitalWrite(vent,HIGH);
    digitalWrite(residuVent,HIGH);
    etapakm=4;
    ventilador=HIGH;
  }*/
}
//Función para añadir cantidad de segundos al temporizador del abono en modo manual
void ponerTempAbono() {
  static int conteo = 0;
  static int contador = 5;
  static char temporizador[4];//3 dígitos + '\0'
  static bool esperandoConfirmacion=false;
  static bool pantallaInicialMostrada=false;
  temporizadorAbono=map (tempAbono,0,1,0,1000);
  if (entradaTemp==HIGH) {
    char tecla=customKeypad.getKey();
    if (!esperandoConfirmacion){
      // Mostrar solo una vez el texto inicial
      if (pantallaInicialMostrada==false){
        lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Anote segundos");
        pantallaInicialMostrada=true;
      }
      if (tecla && conteo<3 && isDigit(tecla)){
        temporizador[conteo]=tecla;
        lcd.setCursor(contador,1);
        lcd.print(tecla);
        conteo++;
        contador++;
      }
      if (conteo==3){
        temporizador[3]='\0';
        lcd.clear();
        lcd.setCursor(2,0);
        lcd.print("¿Guardar?");
        lcd.setCursor(0,1);
        lcd.print("1:Sí");
        lcd.setCursor(12,1);
        lcd.print("2:No");
        esperandoConfirmacion=true;
      }
    } 
    else{
      if (tecla=='1'){
        tempAbono=atoi(temporizador);
        entradaTemp=LOW;
        //Reset
        conteo=0;
        contador=5;
        memset(temporizador,0,sizeof(temporizador));
        esperandoConfirmacion=false;
        pantallaInicialMostrada=false;
        lcd.clear();
      }
      if (tecla=='2'){
        //Reiniciar para volver a ingresar
        conteo=0;
        contador=5;
        memset(temporizador, 0,sizeof(temporizador));
        esperandoConfirmacion=false;
        pantallaInicialMostrada=false;
        lcd.clear();
        entradaTemp=HIGH;
      }
    }
  }
}

void loop(){
  automan();
  while (automatico==HIGH){
    automan();
    mostrarEtapa();
    Leerdatosluz();
    leerDHT();
    leerHumedad();
    sacarErrorHumS();
    mostrarPID();
    motorcitos();
    mostrarAuto();
    nivelAgua();
    entradaTemp=LOW;
    ponerTempAbono();
  }
  while (manual==HIGH){
    automan();
    if((seguridad==LOW)&&(unavez==HIGH)&&(conmutado==HIGH)){
      contrasena(clave);
      conmutado=LOW;
      inf=1;
      entradaTemp=HIGH;
      lcd.clear();
      tiempoSeguridad=millis();
    }//En Manual y se haya puesto la contraseña una sola vez, habiendo terminado el temporizador
    if (((millis()-tiempoSeguridad)>=20000)&&((seguridad==HIGH)||(conmutado==HIGH))){
      conmutado=LOW;
      seguridad=LOW;
      entradaTemp=LOW;
      tiempoSeguridad=millis();
    }//Temporizador para reactivar la seguridad del modo manual
    if (entradaTemp==HIGH){
      lcd.setCursor(0,0);
      lcd.print("1:TempAbo");
      opcion = customKeypad.getKey();
      if (opcion == '1'){
        lcd.clear();
        ponerTempAbono();
      }
    }
  }
}

void mostrarAuto(){
  switch (etapa){
    //etapa 0 
    case 0: 
    lcd.setCursor(0,0);
    lcd.print("1:Hum_Amb");
    lcd.setCursor(0,1);
    lcd.print("2:Hum_Suelo");
    lcd.setCursor(11,0);
    lcd.print("*:SIG");
    opcion = customKeypad.getKey();
    if (opcion == '*'){
      lcd.clear();
      etapa = 1;
      tiempo = millis();
    }
    if (opcion == '1'){
      lcd.clear();
      etapa = 3;
      tiempo = millis();
    }
    if (opcion == '2'){
      lcd.clear();
      etapa = 4;
      tiempo = millis();
    }
    break;
      
    case 1:
    lcd.setCursor(0,0);
    lcd.print("3:Temp");
    lcd.setCursor(0,1);
    lcd.print("4:Luz");
    lcd.setCursor(11,0);
    lcd.print("*:SIG");
    lcd.setCursor(11,1);
    lcd.print("#:ANT");
    opcion = customKeypad.getKey();
     if (opcion == '#'){
      lcd.clear();
      etapa = 0; 
      tiempo = millis();
    }
    if (opcion == '*'){
      lcd.clear();
      etapa = 2;
      tiempo = millis();
    }
    if (opcion == '3'){
      lcd.clear();
      etapa = 5;
      tiempo = millis();
    }
    if (opcion == '4'){
      lcd.clear();
      etapa = 6;
      tiempo = millis();
    }
    break;

    case 2:
    lcd.setCursor(0,0);
    lcd.print("5:Nivel Agua");
    lcd.setCursor(0,1);
    lcd.print("6:Salidas");
    lcd.setCursor(11,1);
    lcd.print("#:ANT");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 1;
      tiempo = millis();
    }
    if (opcion == '5'){
      lcd.clear();
      etapa = 7;
      tiempo = millis();
    }
    if (opcion == '6'){
      lcd.clear();
      etapa = 12;
      tiempo = millis();
    }
    break;

    case 3:
    lcd.setCursor(0,0);
    lcd.print("Humedad Ambiente");
    lcd.setCursor(5,1);
    lcd.print(h);
    lcd.print(" %");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 0;
      tiempo = millis();
    }
    break;
      
    case 4:
    lcd.setCursor(1,0);
    lcd.print("Humedad Suelo");
    lcd.setCursor(5,1);
    opcion = customKeypad.getKey();
    if ((millis()-tiempoLcd)>=retardo){
      lcd.print(humedad);
      lcd.print(" %");
      tiempoLcd=millis();
    }
    if (opcion == '#'){
      lcd.clear();
      etapa = 0;
      tiempo = millis();
    }
    break;

    case 5:
    lcd.setCursor(2,0);
    lcd.print("Temperatura");
    lcd.setCursor(5,1);
    lcd.print(t);
    lcd.write(byte(223));
    lcd.print("C");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 1;
      tiempo = millis();
    }
    break;

    case 6:
    lcd.setCursor(6,0);
    lcd.print("Luz");
    opcion = customKeypad.getKey();
    if ((millis()-tiempo)>200){
      luz=Leerdatosluz();
      lcd.clear();
      lcd.setCursor(6,1);
      lcd.print(luz);
      lcd.print(" %");
      tiempo = millis();
    }
    if (opcion == '#'){
      lcd.clear();
      etapa = 1;
      tiempo = millis();
    }
    break;

    case 7:
    lcd.setCursor(0,0);
    lcd.print("1:N.Agua");
    lcd.setCursor(0,1);
    lcd.print("2:N.Abono");
    lcd.setCursor(11,0);
    lcd.print("*:SIG");
    lcd.setCursor(11,1);
    lcd.print("#:ANT");
    opcion = customKeypad.getKey();
     if (opcion == '#'){
      lcd.clear();
      etapa = 2; 
      tiempo = millis();
    }
    if (opcion == '*'){
      lcd.clear();
      etapa = 8;
      tiempo = millis();
    }
    if (opcion == '1'){
      lcd.clear();
      etapa = 9;
      tiempo = millis();
    }
    if (opcion == '2'){
      lcd.clear();
      etapa = 10;
      tiempo = millis();
    }
    break;

    case 8:
    lcd.setCursor(0,0);
    lcd.print("3:N.Agua Resid");
    lcd.setCursor(11,1);
    lcd.print("#:ANT");
    opcion = customKeypad.getKey();
     if (opcion == '#'){
      lcd.clear();
      etapa = 7; 
      tiempo = millis();
    }
    if (opcion == '3'){
      lcd.clear();
      etapa = 11;
      tiempo = millis();
    }
    break;

    case 9:
    lcd.setCursor(2,0);
    lcd.print("Nivel Agua");
    lcd.setCursor(5,1);
    lcd.print(tanque1);
    lcd.setCursor(8,1);
    lcd.print("%");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 7;
      tiempo = millis();
    }
    break;

    case 10:
    lcd.setCursor(2,0);
    lcd.print("Nivel Abono");
    lcd.setCursor(5,1);
    lcd.print(tanque2);
    lcd.setCursor(8,1);
    lcd.print("%");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 7;
      tiempo = millis();
    }
    break;

    case 11:
    lcd.setCursor(1,0);
    lcd.print("Nivel Residual");
    lcd.setCursor(5,1);
    lcd.print(tanque3);
    lcd.setCursor(8,1);
    lcd.print("%");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 8;
      tiempo = millis();
    }
    break;

    case 12:
    lcd.setCursor(0,0);
    lcd.print("1:M.Agua");
    lcd.setCursor(0,1);
    lcd.print("2:M.Abono");
    lcd.setCursor(11,0);
    lcd.print("*:SIG");
    lcd.setCursor(11,1);
    lcd.print("#:ANT");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 2;
      tiempo = millis();
    }
    if (opcion == '*'){
      lcd.clear();
      etapa = 13;
      tiempo = millis();
    }
    if (opcion == '1'){
      lcd.clear();
      etapa = 14;
      tiempo = millis();
    }
    if (opcion == '2'){
      lcd.clear();
      etapa = 15;
      tiempo = millis();
    }
    break;

    case 13:
    lcd.setCursor(0,0);
    lcd.print("3:M.Residual");
    lcd.setCursor(0,1);
    lcd.print("4:M.Vent");
    lcd.setCursor(11,1);
    lcd.print("#:ANT");
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 12;
      tiempo = millis();
    }
    if (opcion == '3'){
      lcd.clear();
      etapa = 16;
      tiempo = millis();
    }
    if (opcion == '4'){
      lcd.clear();
      etapa = 17;
      tiempo = millis();
    }
    break;

    case 14:
    lcd.setCursor(2,0);
    lcd.print("Motor Agua");
    if (digitalRead(agua)==HIGH){
      lcd.setCursor(4,1);
      lcd.print("Encendido");
    }
    if (digitalRead(agua)==LOW){
      lcd.setCursor(5,1);
      lcd.print("Apagado");
    }
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 12;
      tiempo = millis();
    }
    break;

    case 15:
    lcd.setCursor(2,0);
    lcd.print("Motor Abono");
    if (digitalRead(abono)==HIGH){
      lcd.setCursor(4,1);
      lcd.print("Encendido");
    }
    if (digitalRead(abono)==LOW){
      lcd.setCursor(5,1);
      lcd.print("Apagado");
    }
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 12;
      tiempo = millis();
    }
    break;

    case 16:
    lcd.setCursor(0,0);
    lcd.print("M. Agua Residual");
    if (digitalRead(aguaResi)==HIGH){
      lcd.setCursor(4,1);
      lcd.print("Encendido");
    }
    if (digitalRead(aguaResi)==LOW){
      lcd.setCursor(5,1);
      lcd.print("Apagado");
    }
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 13;
      tiempo = millis();
    }
    break;

    case 17:
    lcd.setCursor(1,0);
    lcd.print("M. Ventilador");
    if (digitalRead(vent)==HIGH){
      lcd.setCursor(4,1);
      lcd.print("Encendido");
    }
    if (digitalRead(vent)==LOW){
      lcd.setCursor(5,1);
      lcd.print("Apagado");
    }
    opcion = customKeypad.getKey();
    if (opcion == '#'){
      lcd.clear();
      etapa = 13;
      tiempo = millis();
    }
    break;
  }
}
