/*
** Fichero: cliente.c
** Autores:
** Javier Sánchez Cacho DNI 70959150X
** Marcos Rivas Kyoguro DNI 70962760D
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#define PUERTO 59150
#define TAM_BUFFER 516

int main(int argc, char *argv[]) {
	
	
    int s;  
    struct addrinfo hints, *res;
    struct sockaddr_in midir_in;
    struct sockaddr_in servdir_in;
    int addrlen, len, len1, errcode;
    char buf[TAM_BUFFER];
    int codigoOperacion;
    int contPeticiones = 0;
    
    FILE *file;
    
    
    
    if((file = fopen(argv[3], "r")) == NULL){
    	
    	perror("ERROR: No se pudo abrir el archivo ordenes*.txt");
    	return -1;
    	
	}

    if (argc != 4) {
    	
        fprintf(stderr, "Uso: %s <servidor> <protocolo> <ficheroEntrada.txt>\n", argv[0]);
        return -2;
        
    }

    if (strcmp(argv[2], "TCP") == 0) {						        // CLIENTE TCP

        s = socket(AF_INET, SOCK_STREAM, 0);
        
        if (s == -1) {
            perror(argv[0]);
            fprintf(stderr, "ERROR: %s: no se pudo crear el socket TCP\n", argv[0]);
            exit(1);
        }

        memset((char *)&midir_in, 0, sizeof(struct sockaddr_in));
        memset((char *)&servdir_in, 0, sizeof(struct sockaddr_in));

        servdir_in.sin_family = AF_INET;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        
        
        if ((errcode = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        	
            fprintf(stderr, "ERROR: %s: No se pudo resolver la IP de %s\n", argv[0], argv[1]);
            exit(1);
            
        } else {
        	
            servdir_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
            
        }
        
        freeaddrinfo(res);

        servdir_in.sin_port = htons(PUERTO);


        if (connect(s, (const struct sockaddr *)&servdir_in, sizeof(struct sockaddr_in)) == -1) {
        	
            perror(argv[0]);
            fprintf(stderr, "ERROR: %s: No se pudo conectar con el servidor\n", argv[0]);
            exit(1);
            
        }

        addrlen = sizeof(struct sockaddr_in);
        
        if (getsockname(s, (struct sockaddr *)&midir_in, &addrlen) == -1) {
        	
            perror(argv[0]);
            fprintf(stderr, "ERROR: %s: No se pudo obtener el socket addr\n", argv[0]);
            exit(1);
            
        }
		
		
		len = recv(s, buf, TAM_BUFFER - 1, 0);
        
		if (len == -1) {
       
        		perror("recv");
				exit(1);
        
		}
        		
		if (strncmp(buf, "220", 3) == 0) {												//EL CLIENTE ESPERA A QUE EL SERVIDOR ESTÉ PREPARADO
        		
				while (fgets(buf, sizeof(buf), file)) {
    	    
				//		printf("%s", buf);

		        		if (send(s, buf, strlen(buf), 0) == -1) {
								perror("ERROR: send_cliente");
								exit(1);
		        		}
        
        

				        if(recv(s, buf, TAM_BUFFER - 1, 0) == -1){
				        	
				        		perror("ERROR: recv_cliente");
    		    		        exit(1);
						}
        

        		
														//EL CLIENTE CONFIRMA QUE EL SERVIDOR SE CIERRA PARA CERRARSE
            			if (strncmp(buf, "221", 3) == 0) {
            					fprintf(stderr, "Servicio cerrado\n");
            					
								close(s);																
						}		


    			   //    	sleep(1);
        
				}
		
            
        		fclose(file);

		   
		}else{
            	fprintf(stderr, "ERROR: %s: El servicio no está preparado\n", argv[0]);
		}
    			
	
    
        
        
    } else if (strcmp(argv[2], "UDP") == 0) {
        
        
        if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        	
            perror(argv[0]);
            fprintf(stderr, "ERROR: %s: no se pudo crear el socket\n", argv[0]);
            exit(1);
        }




        memset((char *)&servdir_in, 0, sizeof(struct sockaddr_in));

        servdir_in.sin_family = AF_INET;
        servdir_in.sin_port = htons(PUERTO);


        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        
        
        if ((errcode = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        	
            fprintf(stderr, "ERROR: %s: No se pudo resolver la IP de %s\n", argv[0], argv[1]);
            exit(1);
            
        } else {
        	
            servdir_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
            
        }
        
        freeaddrinfo(res);

        
        addrlen = sizeof(servdir_in);
        
        if (getsockname(s, (struct sockaddr *)&midir_in, &addrlen) == -1) {
        	
            perror(argv[0]);
            fprintf(stderr, "ERROR: %s: No se pudo obtener el socket addr\n", argv[0]);
            exit(1);
            
        }


        while (fgets(buf, sizeof(buf), file)) {
            

            //	printf("%s", buf);

			
            	if(sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&servdir_in, sizeof(struct sockaddr_in)) == -1){
            		
            			perror("ERROR: sento_cliente");
            			
				}

            	if(recvfrom(s, buf, TAM_BUFFER, 0, (struct sockaddr *)&servdir_in, &addrlen) == -1){
            		
            		   	perror("ERROR: recvfrom_cliente");
            		   	
				}
            	

    	        
            			if (strncmp(buf, "221", 3) == 0) {
            					
								fprintf(stderr, "Servicio cerrado\n");
            					
								close(s);																
																	
						}	
				

        	   	//sleep(1);

        }
        
        fclose(file);

     
	} else {
        fprintf(stderr, "Protocolo no reconocido. Use TCP o UDP.\n");
        exit(1);
    }

    return 0;
}

