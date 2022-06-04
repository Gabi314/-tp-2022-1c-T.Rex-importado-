#include "funcionesConsola.h"
// ./home/utnso/tp-2022-1c-T.Rex/pseudocodigo

int main(int argc, char** argv) {

	logger = iniciar_logger();

	if(argc < 2) {
    	log_error(logger,"Falta un parametro");
    	return EXIT_FAILURE;
	}

	int conexion;
	char* ip;
	char* puerto;


	FILE* archivo = fopen(argv[2], "r");

	if(archivo == NULL){
		log_error(logger,"No se lee el archivo");
	}else{
		log_info(logger,"Se leyo el archivo correctamente");
	}

	//char* contenido = NULL;
	//size_t len = 0;

	 struct stat sb;
	 stat(argv[2], &sb);

	 char *contenido = malloc(sb.st_size);
	 t_list* instrucciones = malloc(sb.st_size);
	 instrucciones = list_create();

	 while (fscanf(archivo, "%[^\n] ", contenido) != EOF) {

	     //log_info(logger,unElemento);
	     list_add(instrucciones,contenido);

	 }


	/*while (getline(&contenido, &len, archivo) != -1) {
		list_add(instrucciones,contenido);
		//log_info(logger,"%s",contenido);

	}*/
	//for(int i=0; i < instrucciones->elements_count; i++){
		log_info(logger,list_get(instrucciones,0));
	//}

	//log_info(logger,"Las instrucciones son: %s",instrucciones);
	//char** instruccionesSeparadas = string_split(instrucciones,"\n");

	config = iniciar_config();

	ip = config_get_string_value(config,"IP_KERNEL");
	puerto = config_get_string_value(config,"PUERTO_KERNEL");

	// Creamos una conexión hacia el servidor
    conexion = crear_conexion(ip, puerto);

	// Armamos y enviamos el paquete
	paquete(conexion,"hola");

	fclose(archivo);
	//free(contenido);
	terminar_programa(conexion, logger, config);


}


t_log* iniciar_logger(void){
	return log_create("./consola.log","CONSOLA",true,LOG_LEVEL_INFO);
}

t_config* iniciar_config(void){
	return config_create("consola.config");
}

void paquete(int conexion, char* instrucciones){

	t_paquete* paquete = crear_paquete();

	// Leemos y esta vez agregamos las lineas al paquete
    agregar_a_paquete(paquete,instrucciones,strlen(instrucciones) + 1);
    //free(instrucciones); tira terrible error si libero instrucciones

    enviar_paquete(paquete,conexion);
    eliminar_paquete(paquete);
}

void terminar_programa(int conexion, t_log* logger, t_config* config){
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config)
	  con las funciones de las commons y del TP mencionadas en el enunciado */
	log_destroy(logger);
	config_destroy(config);

	liberar_conexion(conexion);
}
