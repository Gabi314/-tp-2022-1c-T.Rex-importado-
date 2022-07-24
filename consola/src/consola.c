#include "funcionesConsola.h"
// ./home/utnso/tp-2022-1c-T.Rex/pseudocodigo

int main(int argc, char *argv[]) {

	logger = iniciar_logger();

	if(argc < 2) {
    	log_error(logger,"Falta un parametro");
    	return EXIT_FAILURE;
	}

	int conexion;
	char* ip;
	char* puerto;

	config = iniciar_config();

	ip = config_get_string_value(config,"IP_KERNEL");
	puerto = config_get_string_value(config,"PUERTO_KERNEL");

	// Creamos una conexión hacia el servidor
	conexion = crear_conexion(ip, puerto);

	//Armamos y enviamos el paquete
	t_paquete* paquete = crear_paquete();


	FILE* archivo = fopen(argv[1], "r");

	if(archivo == NULL){
		log_error(logger,"No se lee el archivo");
	}else{
		log_info(logger,"Se leyo el archivo correctamente");
	}

	//char * contenido = NULL;
	//size_t len = 0;
	//ssize_t read;

	struct stat sb;
	stat(argv[1], &sb);
	char * contenido = malloc(sb.st_size);;

	char** lineasDeInstrucciones;

	 while (fscanf(archivo, "%[^\n] ", contenido) != EOF) {

		 instrucciones* instruccion = malloc(sizeof(instrucciones));

		 lineasDeInstrucciones = string_split(contenido, " ");

		 instruccion->identificador = malloc(strlen(lineasDeInstrucciones[0])+1);
		 strcpy(instruccion->identificador, lineasDeInstrucciones[0]);

		 dividirInstruccionesAlPaquete(logger,paquete,lineasDeInstrucciones,instruccion);

		 int contadorDeLineas = 0;

		 while (contadorDeLineas <= string_array_size(lineasDeInstrucciones)){
			 free(lineasDeInstrucciones[contadorDeLineas]);
			 contadorDeLineas++;
		 }

		 free(lineasDeInstrucciones);
		 free(instruccion->identificador);
		 free(instruccion);
	 }

	//log_info(logger,"Las instrucciones recibidas son: \n%s",instrucciones);
	fclose(archivo);
	if (contenido) //valida si contenido es NULL
	free(contenido);

	// Enviamos el paquete
	enviar_paquete(paquete,conexion);
	eliminar_paquete(paquete);

	terminar_programa(conexion, logger, config);


}

void dividirInstruccionesAlPaquete(t_log* logger,t_paquete* paquete,char** lineasDeInstrucciones,instrucciones* instruccion){

	if (!strcmp(lineasDeInstrucciones[0],"NO_OP")){// lineasDeInstrucciones,instruccion

		int contadorNO_OP = 0;

		while (contadorNO_OP < atoi(lineasDeInstrucciones[1])){//["NO_OP","5"]

			log_info(logger,"instruccion: %s\n",instruccion->identificador);//puede ser un log
			agregar_a_paquete(paquete, instruccion, strlen(instruccion->identificador)+1);
			contadorNO_OP++;
		}
	}else if (!strcmp(lineasDeInstrucciones[0],"I/O") || !strcmp(lineasDeInstrucciones[0],"READ") || !strcmp(lineasDeInstrucciones[0],"WRITE") || !strcmp(lineasDeInstrucciones[0],"COPY") || !strcmp(lineasDeInstrucciones[0],"EXIT")){
		log_info(logger,"instruccion: %s\n",instruccion->identificador);

		if (!strcmp(lineasDeInstrucciones[0],"I/O") || !strcmp(lineasDeInstrucciones[0],"READ") || !strcmp(lineasDeInstrucciones[0],"WRITE") || !strcmp(lineasDeInstrucciones[0],"COPY")){
			instruccion -> parametros[0] = atoi(lineasDeInstrucciones[1]);

			if (!strcmp(lineasDeInstrucciones[0],"WRITE") || !strcmp(lineasDeInstrucciones[0],"COPY")){
				instruccion -> parametros[1] = atoi(lineasDeInstrucciones[2]);
			}
		}// se puede implementar un switch?

		agregar_a_paquete(paquete, instruccion, strlen(instruccion->identificador)+1);
	}

}


t_log* iniciar_logger(void){
	return log_create("./consola.log","CONSOLA",true,LOG_LEVEL_INFO);
}

t_config* iniciar_config(void){
	return config_create("consola.config");
}


void terminar_programa(int conexion, t_log* logger, t_config* config){
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config)
	  con las funciones de las commons y del TP mencionadas en el enunciado */
	log_destroy(logger);
	config_destroy(config);

	liberar_conexion(conexion);
}