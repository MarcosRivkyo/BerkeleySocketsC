/*
** Fichero: servidor.c
** Autores:
** Javier Sánchez Cacho DNI 70959150X
** Marcos Rivas Kyoguro DNI 70962760D
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>


#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>


#define PUERTO 59150
#define ADDRNOTFOUND 0xffffffff
#define BUFFERSIZE 516
#define TAM_BUFFER 516
#define MAXHOST 128
#define MAX_TRIES 4
#define NUM_PREGUNTAS 20
#define MAX_CLIENTS 5


extern int errno;


typedef struct {
    const char* pregunta;
    int solucion;
} Pregunta;


int FIN = 0;

static int cont = 0;
int acierto = 0;
int intentos;
static int contPregunta = 0;

int semid;
int semid_UDP;


union semun {
        int val;
        struct semid_ds *buf;
    //    ushort_t        *array;
};

union semun sem1;

Pregunta preguntas[NUM_PREGUNTAS] = {
    {"¿A cómo está hoy la prima de riesgo?", 110},
    {"¿Cuántos estados tiene EEUU?", 51},
    {"¿Cuántos lados tiene un cuadrado?", 4},
    {"¿Cuál es la raíz cuadrada de 25?", 5},
    {"¿En qué año nació Albert Einstein?", 1879},
    {"¿Cuánto es 7 + 4?", 11},
	{"¿Cuál es la raíz cuadrada de 25?", 5},
	{"Si tengo 3 manzanas y doy 2, ¿cuántas me quedan?", 1},
	{"¿Cuánto es 8 multiplicado por 6?", 48},
	{"Resta 9 a 15.", 6},
	{"Si tengo $20 y compro algo por $8, ¿cuánto dinero me queda?", 12},
	{"¿Cuántos lados tiene un cuadrado?", 4},
	{"Suma los primeros 5 números naturales.", 15},
	{"Si un día tiene 24 horas, ¿cuántas horas son la mitad de un día?", 12},
	{"¿Cuántos meses tienen 28 días?", 12},
	{"Si tengo 5 amigos y cada uno tiene 3 caramelos, ¿cuántos caramelos hay en total?", 15},
	{"Si el triple de un número es 21, ¿cuál es el número?", 7},
	{"¿Cuántos grados hay en un triángulo?", 180},
	{"Si un libro tiene 300 páginas y leo 30 páginas cada día, ¿en cuántos días lo terminaré?", 10},
	{"¿Cuál es el resultado de 2 elevado a la 3ra potencia?", 8}
};




void servidorTCP(int s, struct sockaddr_in peeraddr_in, FILE *fichero, FILE *file_cliente);
void servidorUDP(int s, struct sockaddr_in clientaddr_in, FILE *fichero, FILE *fichero_cliente);
void error(char *message);
void finalizar();

void registrarSeparacion(FILE *fichero);
void registrarEvento(FILE *fichero, const char *evento, struct sockaddr_in clientaddr_in, const char *buf, const char *protocolo);


int addrlen;
int connected_clients = 0;



void iniciar_semaforo(int num_semaforo, union semun val_semaforo)
{
	
	
		
		if(semctl(semid, num_semaforo, SETVAL, val_semaforo) == -1)
		{
			perror("ERROR: No se puede inicializar el semaforo.");
			exit(-2);
		}

		
}

void cerrar_semaforo(int id_semaforo) {
	
    if (semctl(id_semaforo, 0, IPC_RMID) == -1) {
        perror("ERROR: No se pudo cerrar el semáforo.");
        exit(-1);
    }
    
}




int sem_wait(int id_semaforo, int num_semaforo)
{
		struct sembuf sops;
	
		sops.sem_num = num_semaforo;
		sops.sem_op = -1;
		sops.sem_flg = 0;
	
		if(semop(id_semaforo, &sops, 1) == -1){
			
			perror("ERROR: No se ha podido hacer la operacion WAIT.");
			return 1;
			
		}
		
		return 0;
}



int sem_signal(int id_semaforo, int num_semaforo)
{
	
        struct sembuf sops;

        sops.sem_num = num_semaforo;
        sops.sem_op = 1;
        sops.sem_flg = 0;

        if(semop(id_semaforo, &sops, 1) == -1){
			perror("ERROR: No se ha podido hacer la operacion SIGNAL.");
			return 1;
			
		}
        
		return 0;
}


int main(int argc, char *argv[]) {
    int s_TCP, s_UDP;
    int ls_TCP, ls_UDP;
    int cc;
    struct sockaddr_in midir_in;
    struct sockaddr_in clientaddr_in;
    fd_set readmask;
    int numfds, s_mayor;
    char buffer[BUFFERSIZE];
    struct sigaction vec;
    FILE *fichero = fopen("peticiones.log", "a+");
    
    
  

	if((semid=semget(IPC_PRIVATE, 1, IPC_CREAT | 0600)) == -1)
	{
		perror("ERROR: No se han podido crear los semaforos.");
		return 1;
	}
	
	if((semid_UDP=semget(IPC_PRIVATE, 1, IPC_CREAT | 0600)) == -1)
	{
		perror("ERROR: No se han podido crear los semaforos.");
		return 1;
	}

	sem1.val = 1;

	iniciar_semaforo(0, sem1);				



	
	if (fichero == NULL) {
        perror("Error al abrir el fichero .log");
        exit(1);
    }
    

    



    if ((ls_TCP = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: No ha sido posible crear el socket ls_TCP\n", argv[0]);
        exit(1);
    }
    


    memset((char *)&midir_in, 0, sizeof(struct sockaddr_in));
    memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));
    addrlen = sizeof(struct sockaddr_in);
    midir_in.sin_family = AF_INET;
    midir_in.sin_addr.s_addr = INADDR_ANY;
    midir_in.sin_port = htons(PUERTO);

	if (bind(ls_TCP, (struct sockaddr *) &midir_in, sizeof(struct sockaddr_in)) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: No ha sido posible asociar el socket ls_TCP a midir_in\n", argv[0]);
        exit(1);
    }
    
    

    

    if (listen(ls_TCP, 5) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: No ha sido posible escuchar\n", argv[0]);
        exit(1);
    }
    


    if ((s_UDP = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: No ha sido posible crear el socket UDP\n", argv[0]);
        exit(1);
    }
    


    
    memset((char *)&midir_in, 0, sizeof(struct sockaddr_in));
    memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));
    addrlen = sizeof(struct sockaddr_in);
    midir_in.sin_family = AF_INET;
    midir_in.sin_addr.s_addr = INADDR_ANY;
    midir_in.sin_port = htons(PUERTO);
    

    if (bind(s_UDP, (struct sockaddr *)&midir_in, sizeof(struct sockaddr_in)) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: No ha sido posible asociar el UDP\n", argv[0]);
        exit(1);
    }


    setpgrp();

    struct sigaction sa = {.sa_handler = SIG_IGN};

    switch (fork()) {
        case -1:
            perror(argv[0]);
            fprintf(stderr, "%s: No ha sido posible hacer el fork del proceso demonio\n", argv[0]);
            exit(1);
        case 0:
            fclose(stdin);
            fclose(stderr);
            if (sigaction(SIGCHLD, &sa, NULL) == -1) {
                perror(" sigaction(SIGCHLD)");
                fprintf(stderr, "%s: No ha sido posible registrar la señal SIGCHLD\n", argv[0]);
                exit(1);
            }

            vec.sa_handler = finalizar;
            vec.sa_flags = 0;

            if (sigaction(SIGTERM, &vec, (struct sigaction *)0) == -1) {
                perror(" sigaction(SIGTERM)");
                fprintf(stderr, "%s: No ha sido posible registrar la señal SIGTERM\n", argv[0]);
                exit(1);
            }



            while (!FIN) {
            	


                FD_ZERO(&readmask);
                FD_SET(ls_TCP, &readmask);
                FD_SET(s_UDP, &readmask);

                if (ls_TCP > s_UDP) {
                    s_mayor = ls_TCP;
                } else {
                    s_mayor = s_UDP;
                }

                if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                    if (errno == EINTR) {
                        FIN = 1;
                        close(ls_TCP);
                        close(s_UDP);
                        perror("\nFinalizando el servidor. Señal recibida en elect\n");
                    }
                } else {
                    if (FD_ISSET(ls_TCP, &readmask)) {
                    	
                    	
                    	
                        if ((s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen)) == -1) {
							exit(1);
                        }
                        
			
                        
			            socklen_t len_client = sizeof(clientaddr_in);
            
						if (getpeername(s_TCP, (struct sockaddr*)&clientaddr_in, &len_client) == -1) {
    							perror("getpeername");
    							exit(1);
						}	

						int puerto_efimero_cliente = ntohs(clientaddr_in.sin_port);

			            char filename[30];  
            			snprintf(filename, sizeof(filename), "TCP_%d.txt", puerto_efimero_cliente);

            			FILE* cliente_file = fopen(filename, "a+");
            			
						if (cliente_file == NULL) {
                			perror("Error al abrir el archivo del cliente");
                			exit(1);
            			}
                        

                       switch (fork()) {
                            case -1:
                                exit(1);
                            case 0:
                            	
                            	

                    		servidorTCP(s_TCP, clientaddr_in, fichero, cliente_file);
		                    fclose(cliente_file);
        		            exit(0);
                    
                            default:
                                close(s_TCP);
            					fclose(cliente_file); 
                                break;
                        }
                        
                        
                    }
                    if (FD_ISSET(s_UDP, &readmask)) {
						

						
								struct sockaddr_in udp_client_addr;
    							socklen_t udp_client_addrlen = sizeof(udp_client_addr);

    							ssize_t received = recvfrom(s_UDP, buffer, BUFFERSIZE, 0, (struct sockaddr*)&udp_client_addr, &udp_client_addrlen);
    							
    							
    							
								if (received == -1) {
        								perror("recvfrom");
        								continue;  
    							}

    							int udp_puerto_efimero_cliente = ntohs(udp_client_addr.sin_port);

    							char udp_filename[30];
    							snprintf(udp_filename, sizeof(udp_filename), "UDP_%d.txt", udp_puerto_efimero_cliente);
						
    							FILE* udp_cliente_file = fopen(udp_filename, "a+");
    							
								if (udp_cliente_file == NULL) {
        							perror("Error al abrir el archivo del cliente UDP");
        							continue;  
    							}


    							
								servidorUDP(s_UDP, udp_client_addr, fichero, udp_cliente_file);

								fclose(udp_cliente_file);


								

								

        			}
        			
                
            	}
	
            }
       
			close(ls_TCP);
            close(s_UDP);
            fclose(fichero);
            cerrar_semaforo(semid);
            
            printf("\nHa finalizado el programa servidor.\n");
            
        default:
            exit(0);
    }
}




void servidorTCP(int s, struct sockaddr_in clientaddr_in, FILE *fichero, FILE *fichero_cliente) {
	

    int reqcnt = 0;
    char buf[TAM_BUFFER];
    int respuesta;
    
    char hostname[MAXHOST];


    int len, estado;
    long timevar;

    static int primeraPeticion = 0;		
	static int cont = 0;
	int fallo = 0;
	
	char servicioPrep[TAM_BUFFER] = "220 Servicio preparado\r\n";

    char clienteInfo[BUFFERSIZE];
    
    
    inet_ntop(AF_INET, &(clientaddr_in.sin_addr), clienteInfo, MAXHOST);


    estado = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0);
    
	if (estado) {
        if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            perror(" inet_ntop \n");
    }

    time(&timevar);


       	sem_wait(semid,0);

	   	snprintf(buf, sizeof(buf), "220 Servicio preparado\r\n");
	//	printf("%s", buf);
		send(s, buf, strlen(buf), 0);
		fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        fflush(fichero_cliente); 

		registrarSeparacion(fichero);
    	registrarEvento(fichero, "COMUNICACION REALIZADA", clientaddr_in, "", "TCP");

		sem_signal(semid,0);

    
     while (1) {

        len = recv(s, buf, TAM_BUFFER - 1, 0);
        
		reqcnt++;
		
        if (len == -1) {
            perror("Error al recibir datos del cliente");
            exit(EXIT_FAILURE);
        }
        

        if (len == 0) {

            printf("Servicio cerrado\r\n");
            break;

        }

        buf[len] = '\0';
		
		Pregunta cuestion;
		
		//=====================================================================
		//No puede empezarse la comunicación con una órden distinto a HOLA\r\n
		//=====================================================================

		
       if (strcmp(buf, "HOLA\r\n") != 0 && primeraPeticion == 0) 
		{
			sem_wait(semid,0);
			
			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");

			snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
			//printf("%s", buf);
			send(s, buf, strlen(buf), 0);
			fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
	        fflush(fichero_cliente);  

            registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
			
			sem_signal(semid,0);


        }

		//=====================================================================
		//La comunicación debe comenzar con la órden HOLA\r\n
		//=====================================================================


        else if (strcmp(buf, "HOLA\r\n") == 0 && primeraPeticion == 0) 
		{
			
			sem_wait(semid,0);

			
            registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");
			
			
			Pregunta nuevaPregunta = preguntas[cont];
			cuestion = nuevaPregunta;
			intentos = MAX_TRIES;
		    snprintf(buf, sizeof(buf), "250 %s#%d\r\n", nuevaPregunta.pregunta, intentos);
            //printf("%s", buf);
            send(s, buf, strlen(buf), 0);
			fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
            fflush(fichero_cliente);  

            registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
           
		   
		    primeraPeticion = 1;
            cont++;
			sem_signal(semid,0);

        }
        
		//=====================================================================
        //No puede utilizarse la órden HOLA\r\n despues de iniciarse la comunicación
		//=====================================================================

		
		else if (strcmp(buf, "HOLA\r\n") == 0 && primeraPeticion == 1) 
		{
			sem_wait(semid,0);

			
			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");

			
			snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
			//printf("%s", buf);
			send(s, buf, strlen(buf), 0);
			fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
			fflush(fichero_cliente);  

			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");

			sem_signal(semid,0);


        }		
  		//=====================================================================      
        // La orden + solicita otra pregunta después de un acierto o si se agotaron los intentos
		//=====================================================================

        
		else if (strcmp(buf, "+\r\n") == 0){
        	
        	if(intentos == 1 || acierto == 1){
        		
        		sem_wait(semid,0);

        		
        		registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");

        		Pregunta nuevaPregunta = preguntas[cont];
        		cuestion = nuevaPregunta;
				intentos = MAX_TRIES;
				fallo = 0;
                snprintf(buf, sizeof(buf), "250 %s#%d\r\n", nuevaPregunta.pregunta, intentos);
            //	printf("%s", buf);

				send(s, buf, strlen(buf), 0); 
				cont++;
				fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
                fflush(fichero_cliente);  

            	registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
        		
        		sem_signal(semid,0);

        		
			}else{
				
				sem_wait(semid,0);

				
        		registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");
				
				snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
            	send(s, buf, strlen(buf), 0);
            //	printf("%s", buf);
				fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
            	fflush(fichero_cliente);  
            	
				registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");

				sem_signal(semid,0);

			}
        	
        	
		} 
        
        
		else if (strncmp(buf, "RESPUESTA ", 10) == 0) 
		{
			sem_wait(semid,0);

			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");


			
			if(cuestion.solucion == -999){											        // La RESPUESTA enviada no tiene pregunta que responder
				    snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
                    send(s, buf, strlen(buf), 0);
		        //    printf("%s", buf);
		            fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        			fflush(fichero_cliente);  

        			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
					
					sem_signal(semid,0);

			}
			else{
				

            int respuesta;

            if (sscanf(buf + 10, "%d", &respuesta) == 1) 
			{

                if (respuesta == cuestion.solucion && fallo != 1) 							        // La RESPUESTA es correcta
				{

                    snprintf(buf, sizeof(buf), "350 ACIERTO\r\n");
                    send(s, buf, strlen(buf), 0);
		        //    printf("%s", buf);
		            fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        			fflush(fichero_cliente);  

        			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
		            
		            acierto = 1;
		            cuestion.solucion = -999;
		            
					sem_signal(semid,0);

                    
                } else  {
                	
                	
                if(intentos == 1)																	        // El número de reintentos se ha consumido
				{
                    	
					snprintf(buf, sizeof(buf), "375 FALLO\r\n");
                    send(s, buf, strlen(buf), 0);
                //    printf("%s", buf);
					fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        			fflush(fichero_cliente);  

	        		registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
                	
                	fallo = 1;
                	cuestion.solucion = -999;

                	
					sem_signal(semid,0);


				}
                	
                if(respuesta < cuestion.solucion && intentos != 1)											  // La RESPUESTA enviada es menor que la solución
				{
                		
                    intentos = intentos -1;
                    snprintf(buf, sizeof(buf), "354 MAYOR#%d\r\n", intentos);
                    send(s, buf, strlen(buf), 0);
		        //    printf("%s", buf);
		           	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        			fflush(fichero_cliente); 

	        		registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
                		
                	acierto = 0;
                	
                	sem_signal(semid,0);

				}
					
				if(respuesta > cuestion.solucion && intentos!= 1)										        // La RESPUESTA enviada es mayor que la solución
				{
                		
                    intentos = intentos -1;
                    snprintf(buf, sizeof(buf), "354 MENOR#%d\r\n", intentos);						
                    send(s, buf, strlen(buf), 0);
		        //    printf("%s", buf);      
              		fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        			fflush(fichero_cliente);  

	        		registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
                	acierto = 0;
                	sem_signal(semid,0);


				}

                    

                    
                }
                
            }

			else 
			{
					snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");									// Error de sintaxis debido al uso incorrecto de la orden RESPUESTA
                    send(s, buf, strlen(buf), 0);
                //    printf("%s", buf);
                    fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        			fflush(fichero_cliente);  

                    
	        		registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");

                    sem_signal(semid,0);

			}
		}
			
        } 

			//La comunicación se finaliza con la orden ADIOS\r\n
			
		else if(strcmp(buf, "ADIOS\r\n") == 0) 
		{
			sem_wait(semid,0);

			
			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");

			snprintf(buf, sizeof(buf), "221 Cerrando el servicio\r\n");
            send(s, buf, strlen(buf), 0);
        //    printf("%s", buf);
            fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        	fflush(fichero_cliente);  

			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");
            
            sem_signal(semid,0);
            
            break;

            
        }
    
		else 
		{
			sem_wait(semid,0);

			
			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "TCP");							//Fallos generales encontrados 

        	
            snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
            send(s, buf, strlen(buf), 0);
		//    printf("%s", buf);
		    fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
        	fflush(fichero_cliente);  

			
			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "TCP");

			sem_signal(semid,0);

		}
		
		

    
    }
    
    sem_wait(semid,0);

    registrarEvento(fichero, "COMUNICACION FINALIZADA", clientaddr_in, "", "TCP");
	registrarSeparacion(fichero);

	sem_signal(semid,0);
	
	
    close(s);
    time(&timevar);
    // printf("Completado %s puerto %u, %d peticiones, a las %s\n", clienteInfo, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));

	sem_signal(semid,0);


    
}


void servidorUDP(int s,struct sockaddr_in clientaddr_in, FILE *fichero, FILE *fichero_cliente) {


    Pregunta cuestion;

	char buf[BUFFERSIZE];
	int respuesta;
	
	char hostname[MAXHOST];
    int len, estado;
	long timevar;

	
	

	static int cont = 0;
	int reqcnt = 0;
	int primeraPeticion = 0;
	int i = 0;
	int fallo = 0;
	
	
	char servicioPrep [BUFFERSIZE] = "220 Servicio preparado\r\n";
	
	char clienteInfo[BUFFERSIZE];
    
    inet_ntop(AF_INET, &(clientaddr_in.sin_addr), clienteInfo, MAXHOST);


	

    estado = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0);
    if (estado) {
        if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            perror(" inet_ntop \n");
    }

    time(&timevar);
    
    
    sem_wait(semid,0);

    
    snprintf(buf, sizeof(buf), "220 Servicio preparado\r\n");
	//printf("%s", buf);
    sendto(s, buf, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
	
	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
    fflush(fichero_cliente);  


	registrarSeparacion(fichero);
		
    registrarEvento(fichero, "COMUNICACION REALIZADA", clientaddr_in, "", "UDP");

    sem_signal(semid,0);

    

     while (1) {


        
    	len = recvfrom(s, buf, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, &addrlen);
    	
    	if (len == -1) {
            perror("Error al recibir datos del cliente");
            exit(EXIT_FAILURE);
        }
        

        if (len == 0) {
            // Cliente cerró la conexión
            printf("Servicio cerrado\r\n");
            break;
        }
        
        buf[len]='\0';

    	Pregunta cuestion;

    	reqcnt++;
    	
		

		//======================================================		
		//No puede empezarse la comunicación con una órden distinto a HOLA\r\n
		//======================================================


       if (strcmp(buf, "HOLA\r\n") != 0 && primeraPeticion == 0) 
		{
			
			sem_wait(semid,0);

			
			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");

			snprintf(buf, sizeof(buf), "500 Error de sintxis\r\n");
			
		//	printf("%s", buf);
			
            sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
		
			fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
	        fflush(fichero_cliente);  

			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
		
    		sem_signal(semid,0);
    
	 
	    }
        	
		//======================================================
		//La comunicación debe comenzar con la órden HOLA\r\n
		//======================================================

        else if (strcmp(buf, "HOLA\r\n") == 0 && primeraPeticion == 0) 
		{
			
			sem_wait(semid,0);

			
			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");

			
			Pregunta nuevaPregunta = preguntas[cont];
			cuestion = nuevaPregunta;
			intentos = MAX_TRIES;
		    snprintf(buf, sizeof(buf), "250 %s#%d\r\n", nuevaPregunta.pregunta, intentos);
        //    printf("%s", buf);
            sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
            
            fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
	        fflush(fichero_cliente); 

            registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
            
			primeraPeticion = 1;
            cont++;
            
            sem_signal(semid,0);

            
        }
		//======================================================
		//No puede utilizarse la órden HOLA\r\n despues de iniciarse la comunicación
		//======================================================

		
		else if (strcmp(buf, "HOLA\r\n") == 0 && primeraPeticion == 1) 
		{
			
			sem_wait(semid,0);

			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");

			snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
		//	printf("%s", buf);
            sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
            
         	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
	        fflush(fichero_cliente);  

			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");

    		sem_signal(semid,0);


        }		
   
   		//======================================================     
        // La orden + solicita otra pregunta después de un acierto o si se agotaron los intentos
		//======================================================


		else if (strcmp(buf, "+\r\n") == 0){
        	
        	if(intentos == 1 || acierto == 1){
        		
        		sem_wait(semid,0);
        		
        		registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");

        		
        		Pregunta nuevaPregunta = preguntas[cont];
        		cuestion = nuevaPregunta;
                intentos = MAX_TRIES; 
                fallo = 0;
                snprintf(buf, sizeof(buf), "250 %s#%d\r\n", nuevaPregunta.pregunta, intentos);
            //	printf("%s", buf);

            	sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
				cont++;
	           	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
     	        fflush(fichero_cliente);  

				registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
        	        
				sem_signal(semid,0);
		
        		
			}else{
				
				sem_wait(semid,0);

				registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");
			
				
				snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
              	sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
            //	printf("%s", buf);
            	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
		        fflush(fichero_cliente); 

				registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");

				sem_signal(semid,0);

				
			}
        	
        	
		} 
        
        
		else if (strncmp(buf, "RESPUESTA ", 10) == 0) 
		{
			sem_wait(semid,0);

			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");

			if(cuestion.solucion == -999){																// La RESPUESTA enviada no tiene pregunta que responder
				    snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
            		sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
		       //     printf("%s", buf);
				  	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
          	        fflush(fichero_cliente); 

					registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");

		            sem_signal(semid,0);

			}else{
			
			
            if (sscanf(buf + 10, "%d", &respuesta) == 1) 
			{

                if (respuesta == cuestion.solucion && fallo != 1) 
				{

                    snprintf(buf, sizeof(buf), "350 ACIERTO\r\n");
            		sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
		        //    printf("%s", buf);
		        	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
    	        fflush(fichero_cliente);  

					registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
		            		            
		            acierto = 1;
		            cuestion.solucion = -999;
		            
					sem_signal(semid,0);
		            

                    
                } else {
                	
                 if(intentos == 1)
				{
                    	
					snprintf(buf, sizeof(buf), "375 FALLO\r\n");
            		sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
                //    printf("%s", buf);
					fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
		        fflush(fichero_cliente); 

					registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
                	                	
                	fallo = 1;
                	cuestion.solucion = -999;
				
		            sem_signal(semid,0);
				
				}
                	
                if(respuesta < cuestion.solucion && intentos != 1)
				{
                		
                    intentos = intentos -1;
                    snprintf(buf, sizeof(buf), "354 MAYOR#%d\r\n", intentos);
            		sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
		        //    printf("%s", buf);
		        	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
    	        fflush(fichero_cliente); 

					registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
                		                		
                	acierto = 0;
                	
		            sem_signal(semid,0);
                	
				}
					
				if(respuesta > cuestion.solucion && intentos!= 1)
				{
                		
                    intentos = intentos -1;
                    snprintf(buf, sizeof(buf), "354 MENOR#%d\r\n", intentos);
            		sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
		        //    printf("%s", buf);      
	              	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
			        fflush(fichero_cliente);

					registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");

                	acierto = 0;
                	
		            sem_signal(semid,0);
                	

				}

                    

                    
                }
                
            }

			else 
			{
					snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
            		sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
                //    printf("%s", buf);
                	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
    	        fflush(fichero_cliente);  

					registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");

		            sem_signal(semid,0);

			}
		}
			
        } 
		
		//======================================================
		//La comunicación se finaliza con la orden ADIOS\r\n
		//======================================================

			
		else if(strcmp(buf, "ADIOS\r\n") == 0) 
		{
			sem_wait(semid,0);

			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");

			snprintf(buf, sizeof(buf), "221 Cerrando el servicio\r\n");
            sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
        //    printf("%s", buf);
         	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
   	        fflush(fichero_cliente);  

			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
            
		    sem_signal(semid,0);
			

        }
    
		else
		{
			sem_wait(semid,0);

			registrarEvento(fichero, "\tORDEN RECIBIDA", clientaddr_in, buf, "UDP");


            snprintf(buf, sizeof(buf), "500 Error de sintaxis\r\n");
            sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in));
		//    printf("%s", buf);
		 	fwrite(buf, sizeof(char), strlen(buf), fichero_cliente);
   	        fflush(fichero_cliente); 

			registrarEvento(fichero, "\tRESPUESTA ENVIADA", clientaddr_in, buf, "UDP");
			
			sem_signal(semid,0);

			
		}
	
    
    }
    
    sem_wait(semid,0);

    registrarEvento(fichero, "COMUNICACION FINALIZADA", clientaddr_in, "", "UDP");
	registrarSeparacion(fichero);

	sem_signal(semid,0);
	
	
    close(s);

    time(&timevar);
    // printf("Completado %s puerto %u, %d peticiones, a las %s\n", clienteInfo, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
	



}




void error(char *hostname) {
    printf("Conexión con %s abortada debido a un error.\n", hostname);
    exit(1);
}
void finalizar() {
    FIN = 1;
}



void registrarSeparacion(FILE *fichero){
	
	char buffer[BUFFERSIZE];
	
	snprintf(buffer, sizeof(buffer), "##################################################################################################################################################");
	
    fprintf(fichero, "%s\r\n", buffer);
    fflush(fichero);
	
}

void registrarSeparacion2(FILE *fichero, int reqcnt){
	
	char buffer[BUFFERSIZE];
	
	snprintf(buffer, sizeof(buffer), "\t<MENSAJE %d> ===============================================================================", reqcnt+1);
	
    fprintf(fichero, "%s\r\n", buffer);
    fflush(fichero);
	
}

void registrarEvento(FILE *fichero, const char *evento, struct sockaddr_in clientaddr_in, const char *buf, const char *protocolo) 
{
    

    
    char tiempo[30];
    time_t now = time(NULL);
    strftime(tiempo, 30, "%Y-%m-%d %H:%M:%S", localtime(&now));

    
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    
    int result = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);

  	if (result) {
        strcpy(host, "Desconocido");
    }

    
       	fprintf(fichero, "%s [%s]: Host: %s, IP: %s, Protocolo: %s, Puerto: %s, Mensaje: %s\n", 
            evento, tiempo, host, inet_ntoa(clientaddr_in.sin_addr), protocolo, service, buf);

	       fflush(fichero);

	   
}




