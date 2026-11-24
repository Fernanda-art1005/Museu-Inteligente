#include <Wire.h>

const byte pir = 6, led = 7, buzzer = 8, trig = 9, echo = 10, botao = 4;
const unsigned long limiteUs = 30UL * 58;
const unsigned long tempoLuz = 3000;

bool protecao = true, perto = false, alarme = false;
bool luz = false;
int leituraBotao = HIGH, botaoEstavel = HIGH;
int telaAtual = -1;
unsigned long mudouBotao = 0, ultimaMedicao = 0, ultimoMovimento = 0;

void expansor(byte valor) {
  Wire.beginTransmission(0x27);
  Wire.write(valor | 0x08);
  Wire.endTransmission();
}

void nibbleLCD(byte valor) {
  expansor(valor);
  expansor(valor | 0x04);
  delayMicroseconds(1);
  expansor(valor);
  delayMicroseconds(50);
}

void enviarLCD(byte valor, bool caractere) {
  byte modo = caractere ? 1 : 0;
  nibbleLCD((valor & 0xF0) | modo);
  nibbleLCD(((valor << 4) & 0xF0) | modo);
}

void linhaLCD(byte endereco, const char *texto) {
  enviarLCD(endereco, false);
  for (byte coluna = 0; coluna < 16; coluna++) {
    enviarLCD(*texto ? *texto++ : ' ', true);
  }
}

void mostrar(int tela, const char *linha1, const char *linha2) {
  if (tela == telaAtual) return;
  telaAtual = tela;
  linhaLCD(0x80, linha1);
  linhaLCD(0xC0, linha2);
}

void setup() {
  pinMode(pir, INPUT);
  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(botao, INPUT_PULLUP);

  Wire.begin();
  delay(50);
  nibbleLCD(0x30);
  delay(5);
  nibbleLCD(0x30);
  delayMicroseconds(150);
  nibbleLCD(0x30);
  nibbleLCD(0x20);
  enviarLCD(0x28, false);
  enviarLCD(0x0C, false);
  enviarLCD(0x01, false);
  delay(2);
  enviarLCD(0x06, false);
  mostrar(6, "Mona Lisa", "Obra protegida");
}

void atualizarBotao(unsigned long agora) {
  int leitura = digitalRead(botao);
  if (leitura != leituraBotao) {
    leituraBotao = leitura;
    mudouBotao = agora;
  }
  if (leituraBotao == botaoEstavel || agora - mudouBotao < 30) return;
  botaoEstavel = leituraBotao;
  if (botaoEstavel == LOW) {
    protecao = !protecao;
    alarme = false;
    noTone(buzzer);
  }
}

void atualizarRecepcao(unsigned long agora) {
  if (digitalRead(pir) == HIGH) {
    ultimoMovimento = agora;
    luz = true;
  }
  if (luz && agora - ultimoMovimento >= tempoLuz) luz = false;
  digitalWrite(led, luz ? HIGH : LOW);
}

void medirDistancia(unsigned long agora) {
  if (agora - ultimaMedicao < 70) return;
  ultimaMedicao = agora;
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long pulso = pulseIn(echo, HIGH, 24000);
  perto = pulso > 0 && pulso <= limiteUs;
}

void loop() {
  unsigned long agora = millis();
  atualizarBotao(agora);
  atualizarRecepcao(agora);
  medirDistancia(agora);

  if (!protecao) {
    mostrar(7, "Mona Lisa", "Protecao off");
    return;
  }
  if (!perto) {
    if (alarme) {
      noTone(buzzer);
      alarme = false;
    }
    mostrar(6, "Mona Lisa", "Obra protegida");
    return;
  }

  if (alarme) return;
  alarme = true;
  tone(buzzer, 1800);
  mostrar(0, "Alarme acionado", "");
}
