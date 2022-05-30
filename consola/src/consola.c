#include "funcionesConsola.h"

int main(void) {

	int conexion;
	char* ip;
	char* puerto;
	char* instrucciones;

	t_log* logger;
	t_config* config;

	logger = iniciar_logger();

	log_info(logger,"Aca se loggea consola");//(?

	config = iniciar_config();

	ip = config_get_string_value(config,"IP_KERNEL");
	puerto = config_get_string_value(config,"PUERTO_KERNEL");
	instrucciones = config_get_string_value(config,"instrucciones");

	// Creamos una conexión hacia el servidor
    conexion = crear_conexion(ip, puerto);

	// Armamos y enviamos el paquete
	paquete(conexion,instrucciones);

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
