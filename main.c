/*
 * ICDALERT V1.4.c
 *
 * Created: 27/02/2022 06:59:02 p. m.
 * Author : MIGUEL ANGEL MARQUEZ HERNANDEZ
 */ 


#define F_CPU 1000000UL


#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// INSTRUCCIONES I2C
#define I2C_WRITE 0x00
#define I2C_READ 0x01
#define I2C_ACK 0x01
#define I2C_NACK 0x00
#define I2C_START 0x08
#define I2C_RESTART	0x10
#define I2C_SLAVE_WRITE_ACK 0x18
#define I2C_BYTE_TX_ACK	0x28
#define I2C_SLAVE_READ_ACK	0x40

// DIRECCIÓN DEL ACELERÓMETRO
#define MMA845_ADDRESS 0x1C

#define DEBUG_MODE false
#define MAXJERK 1500

float temp = 0;
char serial_buffer[45] = "";

// FUNCIONES I2C
void i2c_init(void);
bool i2c_start_cond(void);
bool i2c_restart_cond(void);
void i2c_stop_cond(void);
bool i2c_write(unsigned char);
unsigned char i2c_read(unsigned char);
bool i2c_tx_address(unsigned char, unsigned char);

// FUNCIONES UART
void serial_char_tx (unsigned char character);
char serial_char_rx(void);
void serial_init (void) ;
void serial_message_tx(char*); 
void serial_newline_tx(void);

void adc_init(void);
int adc_read(char channel);

// INICIALIZACIÓN DEL ACELERÓMETRO
bool MMA845X_init();
void MMA845X_serialdebug_accel(int);

// FUNCIÓN DE LECTURA DE TEMPERATURA
float get_temp(void);

// FUNCIONES DE LECTURA DE ACELERACIÓN
int get_x_accel(void);
int get_y_accel(void);
int get_z_accel(void);

// BANDERAS DE CAÍDA Y FIEBRE
bool temp_alert = false;
bool jerk_alert = false;

int main(void) {
	
	// VALORES DE ACELERACIÓN Y SOBREACELERACIÓN
	int x_accel = 0, y_accel = 0, z_accel = 0, x_jerk = 0, y_jerk = 0, z_jerk = 0; 
	int prevx_accel = 0, prevy_accel = 0, prevz_accel = 0; 

	
	// INICIALIZACIÓN DE LOS PREIFÉRICOS
	adc_init();
	i2c_init();
	MMA845X_init();
	serial_init();
	serial_message_tx("Hola katia como estas \n");
	
	while(1){

		cli();
		temp = get_temp();
		prevx_accel = x_accel;
		prevy_accel = y_accel;
		prevz_accel = z_accel; 
		x_accel = get_x_accel();
		y_accel = get_y_accel();
		z_accel = get_z_accel();
		
		
		x_jerk = x_accel -prevx_accel;
		y_jerk = y_accel -prevy_accel;
		z_jerk = z_accel -prevz_accel;
		
		
		// ALERTA DE CAÍDA
		if( abs(x_jerk) > MAXJERK || abs(y_jerk) > MAXJERK || abs(z_jerk) > MAXJERK){
			jerk_alert = true; 
		}
		sei();
		if (DEBUG_MODE == true){
// 			serial_message_tx("Puerto serie inicializado");
// 			serial_newline_tx();
// 			while(1);
			
// 			strcpy(serial_buffer, "TEMP:, X_ACCEL:, Y_ACCEL:, Z_ACCEL:,");
// 			serial_message_tx(serial_buffer);
// 			serial_newline_tx();
// 			
// 			dtostrf(temp, 2, 3, serial_buffer);
// 			serial_message_tx(serial_buffer);
// 			serial_char_tx(',');
// 			serial_char_tx(' ');
// 			
// 			MMA845X_serialdebug_accel(x_accel);
// 			serial_char_tx(',');
// 			serial_char_tx(' ');
// 			
// 			MMA845X_serialdebug_accel(y_accel);
// 			serial_char_tx(',');
// 			serial_char_tx(' ');
// 			
// 			MMA845X_serialdebug_accel(z_accel);
// 			serial_char_tx(',');
// 			serial_char_tx(' ');
// 				
// 		
			strcpy(serial_buffer, "X_JERK: ");
			serial_message_tx(serial_buffer);
			strcpy(serial_buffer, "");
			itoa(x_jerk, serial_buffer, 10);
			serial_message_tx(serial_buffer);
			serial_char_tx(',');
			serial_char_tx(' ');
		
			strcpy(serial_buffer, "Y_JERK: ");
			serial_message_tx(serial_buffer);
			strcpy(serial_buffer, "");
			itoa(y_jerk, serial_buffer, 10);
			serial_message_tx(serial_buffer);
			serial_char_tx(',');
			serial_char_tx(' ');
		
			strcpy(serial_buffer, "Z_JERK: ");
			serial_message_tx(serial_buffer);
			strcpy(serial_buffer, "");
			itoa(z_jerk, serial_buffer, 10);
			serial_message_tx(serial_buffer);
			serial_char_tx(',');
			serial_char_tx(' ');
			serial_newline_tx();
			

		}
	}
}

void serial_init (void) {
	cli();
	// PASO 1: CONFIGURACION DEL USART COMO UART
	UCSR0C &=~ (1 << UMSEL00);
	UCSR0C &=~ (1 << UMSEL01);

	// PASO 2: DESACTIVAR PARIDAD
	UCSR0C &=~ (1 << UPM00);
	UCSR0C &=~ (1 << UPM01);
	
	// PASO 3: CONFIGURAR SOLO 1 STOP BIT
	UCSR0C &=~ (1 << USBS0);

	// PASO 4: CONFIGURAR EL TAMAÑO DEL PAQUETE COMO 8 BITS
	UCSR0C |= (1 << UCSZ00);
	UCSR0C |= (1 << UCSZ01);
	UCSR0B &=~ (1 << UCSZ02);

	// PASO 5: CONFIGURAR BAUDRATE A 9600
	UCSR0A |= (1 << U2X0);
	UBRR0 = (F_CPU/8/9600) - 1;
	
	// PASO 6: HABILITAR TX Y RX
	UCSR0B |= (1 << RXCIE0);

	// HABILITAR INTERRUPCIONES
	sei();
	UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
}

void serial_char_tx (unsigned char character){
	
	while (!(UCSR0A & (1 << UDRE0)));				// ESPERAR A QUE SE TRANSMITA EL BIT ANTERIOR
	UDR0 = character;
}

char serial_char_rx (void){
	
	while (!(UCSR0A & (1 << RXC0)));				// ESPERAR A QUE LA BANDERA DE RECEPCIÓN SE ACTIVE
	return UDR0;
}

void serial_message_tx(char *message){
	
	while(*message != 0x00){
		serial_char_tx(*message);
		message++; 										
	}

}

void serial_newline_tx(void){

	serial_char_tx('\r');
	serial_char_tx('\n');
}

void adc_init(){
	
	ADMUX &= ~(1 << ADLAR);					// AJUSTAR LA SALIDA A LA DERECHA
	
	ADMUX |=  (1 << REFS0);
	ADMUX &= ~(1 << REFS1);					// REFSN = 01 (AVCC)

	
	ADCSRA |= (1 << ADPS0) | (1 << ADPS1);	// CONFIGURAR PRESCALER /8

}

int adc_read(char channel){
	
	channel &= 0x0F;						// TRUNCAR EL VALOR DE CHANNEL A 4 BITS
	ADMUX |= (channel);						// CONFIGURAR EL CANAL DEL ADC
	ADCSRA |= (1 << ADEN);					// HABILITAR EL ADC
	_delay_us(10);							// ESPERAR A QUE EL ADC SE INICIALICE
	
	ADCSRA |= (1<<ADSC);					// TOMAR MUESTRA DEL ADC
	
	while( !(ADCSRA & (1 << ADIF)) );		// ESPERA A QUE ADIF = 1
	ADCSRA |= (1 << ADIF);					// LIMPIAR BANDERA DE INTERRUPCION
	ADCSRA &= ~(1 << ADEN);					// DESHABILITAR EL ADC
	
	return ADC;
}

void i2c_init(void){
	
	
	// ESTABLECER FRECUENCIA CON LA FÓRMULA
	// SCL_FREQUENCY = CPU CLOCK / (16 + 2(TWBR) * (PRESCALERVALUE))
	
	// ESTABLECER EL PRESCALER EN 00 -> 1
	TWSR &=~ (1 << TWPS0);
	TWSR &=~ (1 << TWPS0);
	
	
	// FACTOR DE DIVISIÓN (TWBR)
	TWBR = 0x16;		// MODIFICADO DE 17 A 0x16
	
	// ENCENDER EL RELOJ PARA I2C (Cogeme Miguel por favor te lo pido :) )
	PRR &=~ (1 << PRTWI);	
	
	
	// F_SCL SERA IGUAL A 456.204Hz ( Neta ya porfa ) (Atte: Josephe)
}

bool i2c_start_cond(void){
	
	TWCR = ((1 << TWINT) | (1 << TWSTA) | (1 << TWEN) );
	while (!(TWCR & (1 << TWINT)));
	if ((TWSR & 0xF8) == I2C_START){
		return false;
	}
	return true;
}

void i2c_stop_cond (void){
	TWCR |= ((1<<TWINT) | (1<<TWSTO) | (1<<TWEN)); 
}

bool i2c_tx_address(unsigned char address, unsigned char action) {
	
	unsigned char cmp = 0;
	address = address << 1;

	if (action == I2C_WRITE){
		address &=~ 1;
		cmp = I2C_SLAVE_WRITE_ACK;
	}
	else{
		address |= 1;
		cmp = I2C_SLAVE_READ_ACK;
	}
	
	TWDR = address;
	TWCR = ((1 << TWINT) | 1 << TWEN);
	
	while (!(TWCR & (1 << TWINT)));
	
	if ((TWSR & 0xF8) == cmp){
		return false;
	}
	
	return true;
	
}

bool i2c_write(unsigned char data){
	
	TWDR = data;
	TWCR = ((1<<TWINT) | (1<<TWEN));
	while(!(TWCR & (1<<TWINT)));
	if ((TWSR & 0xF8) == I2C_BYTE_TX_ACK){
		return false;
	}
	return true;
	
}

bool i2c_restart_cond(){
	TWCR = ((1<<TWINT) | (1<<TWSTA) | (1<<TWEN));

	while(!(TWCR & (1<<TWINT)));

	if ((TWSR & 0xF8) == I2C_RESTART){
		return false;
	}

	return true;
}

unsigned char i2c_read(unsigned char ack_nack){
	TWCR = ((1 << TWINT) | (1 << TWEN) | (ack_nack << TWEA));

	while(!(TWCR & (1<<TWINT)));
	return TWDR;
}

int get_x_accel(void){
	
	int x_msb = 0;
	int x_lsb = 0;
	int x_full = 0;
	
	i2c_start_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_WRITE);
	i2c_write(0x01);
	i2c_restart_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_READ);
	x_msb = i2c_read(I2C_ACK);
	x_lsb = i2c_read(I2C_NACK);
	

	i2c_stop_cond();
	x_full = x_msb << 6 | x_lsb >> 2;
	if (x_full > 0x1FFF){
		x_full|= 0xC000;
	}
	return x_full; 
}

int get_y_accel(void){
	
	char y_msb = 0;
	char y_lsb = 0;
	int y_full = 0;
	
	i2c_start_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_WRITE);
	i2c_write(0x03);
	i2c_restart_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_READ);
	y_msb = i2c_read(I2C_ACK);
	y_lsb = i2c_read(I2C_NACK);
	i2c_stop_cond();
	y_full = y_msb << 6 | y_lsb >> 2;
	if (y_full > 0x1FFF){
		y_full|= 0xC000;
	}

	return y_full; 
}

int get_z_accel(void){

	char z_msb = 0;
	char z_lsb = 0;
	int z_full = 0;
	

	i2c_start_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_WRITE);
	i2c_write(0x05);
	i2c_restart_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_READ);
	z_msb = i2c_read(I2C_ACK);
	z_lsb = i2c_read(I2C_NACK);
	i2c_stop_cond();
	z_full = z_msb << 6 | z_lsb >> 2;
	if (z_full > 0x1FFF){
		z_full|= 0xC000;
	}

	return z_full;
}

void MMA845X_serialdebug_accel(int accel){
		
		unsigned int a,r,b,c,d;
		
		if (accel > 0x1FFF){
			serial_char_tx('-');
			accel = ~accel+'0' -0xC000;
		}
		else{
			serial_char_tx('+');
		}
		
		a = ((accel) / 1000 + 0x30);
		r = (accel) % 1000;
		b = (r / 100)+ '0';
		r %= 100;
		c = (r / 10)+ '0';
		d = (r % 10)+ '0';
		
		serial_char_tx(a);
		serial_char_tx(b);
		serial_char_tx(c);
		serial_char_tx(d);

}

float get_temp(void){
	temp = 0;

	// TOMAR 200 MUESTRAS Y PROMEDIARLAS PARA 
	for (int i = 0; i<200; i++){
		
		temp += ((adc_read(0)*5.0f) / 1023.0f)-0.5;
		_delay_us(50);
	}
	temp = temp/2 - 3.5;

	

	
	// ALERTA DE FIEBRE
	if (temp >= 38){
		temp_alert = true;
	}
	
	return temp;
}

bool MMA845X_init(){
	
	// CONFIGURAR REGISTRO XYZ_DATA_CFG PARA
	// RANGO COMPLETO DE 8G
	cli();
	i2c_start_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_WRITE);
	i2c_write(0x0E);
	i2c_write(0x02);
	i2c_stop_cond();
	
	
	// CONFIGURAR REGISTRO CTRL_REG_1
	// HABILITAR LECTURA DEL ACELERÓMETRO
	i2c_start_cond();
	i2c_tx_address(MMA845_ADDRESS, I2C_WRITE);
	i2c_write(0x2A);
	i2c_write(0x01);
	i2c_stop_cond();
	return true;


}

ISR (USART_RX_vect){
	char request = UDR0;
	
	switch (request){
		
		case 'R': 
			if (jerk_alert == true){
				serial_char_tx('A');
				jerk_alert = false;
			}
			else{
				serial_char_tx('N');
			}
		break;
		
		case 'K':
			if (temp_alert == true){
				serial_char_tx('A');
				temp_alert = false;
			}
			else{
				serial_char_tx('N');
			}
		break;
		
		case 'F':
			dtostrf(temp, 2, 3, serial_buffer);
			serial_message_tx(serial_buffer);
			serial_newline_tx();
		break;
		
		default:
		break;
	}
}