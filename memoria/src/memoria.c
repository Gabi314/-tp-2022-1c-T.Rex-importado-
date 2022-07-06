#include "funcionesMemoria.h"

int marcosTotales = 0;
//Antes de esto debo mandarle a cpu tam_Pagina y cant_entradas_por_pagina
int pid = 0;//Viene de Kernel, para devolver nroDeTabla1erNivel
int entradaDeTablaDe1erNivel = 2; // Viene de cpu(calculado con la dir logica), con esto devuelvo nroTablaDe2doNivel

int entrada = 0;
int contPaginas = 0;

t_list* listaT1nivel;


int main(void) {
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	crearConfiguraciones();

	listaT1nivel = list_create();

	inicializarEstructuras();

	conexionConCpu();

	/*t_primerNivel* unaTablaDe1erNivel = list_get(listaT1nivel,0);
	t_segundoNivel* unaTablaDe2doNivel = list_get(unaTablaDe1erNivel->tablasDeSegundoNivel,3);
	pagina* unaPagina = list_get(unaTablaDe2doNivel->paginas,0);

	log_info(logger,"La pagina ubicada en la primer entrada de la 4ta tablaDeSegundo nivel es %d",unaPagina->numeroDePagina);


	log_info(logger,"El pid es: %d",unaTablaDe1erNivel->pid);
	log_info(logger,"El nro de tabla asociado al pid %d es %d",unaTablaDe1erNivel->pid,buscarNroTablaDe1erNivel(unaTablaDe1erNivel->pid));
	*/
	//free(unaTablaDe1erNivel);


}

void crearConfiguraciones(){
	t_config* config = config_create("memoria.config");

	tamanioDeMemoria = config_get_int_value(config,"TAM_MEMORIA");
	tamanioDePagina = config_get_int_value(config,"TAM_PAGINA");

	entradasPorTabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
	algoritmoDeReemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	marcosPorProceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	retardoSwap = config_get_int_value(config,"RETARDO_SWAP");

}

void inicializarEstructuras(){


	t_primerNivel* tablaPrimerNivel = malloc(sizeof(t_primerNivel));
	tablaPrimerNivel->pid = pid;
	tablaPrimerNivel->tablasDeSegundoNivel = list_create();

	//For tablas 2do nivel
	for(int i=0;i<entradasPorTabla;i++){

		t_segundoNivel* tablaDeSegundoNivel = malloc(sizeof(t_segundoNivel));
		tablaDeSegundoNivel->paginas = list_create();

		cargarPaginas(tablaDeSegundoNivel);
		//tablaDeSegundoNivel->marcos = contadorDeMarcos;

		list_add(tablaPrimerNivel->tablasDeSegundoNivel, tablaDeSegundoNivel);


		//free(tablaDeSegundoNivel);
	}

	contPaginas = 0;


	list_add(listaT1nivel,tablaPrimerNivel);

	//free(tablaPrimerNivel);

	escribirEnSwap(pid);
}

void cargarPaginas(t_segundoNivel* tablaDeSegundoNivel){

	for(int j=0;j<entradasPorTabla;j++){ //crear x cantidad de marcos y agregar de a 1 a la lista
			pagina* unaPagina = malloc(sizeof(pagina));

			unaPagina->numeroDePagina = contPaginas;
			contPaginas++;

			list_add(tablaDeSegundoNivel->paginas,unaPagina);

		}
}

void escribirEnSwap(int pid){

	char* nombreArchivo = string_new();

	char* pidString = string_itoa(pid);
	nombreArchivo = pidString;

	string_append(&nombreArchivo,".swap");

	FILE* archivoSwap = fopen(nombreArchivo, "a+");

	fclose(archivoSwap);

}


int buscarNroTablaDe1erNivel(int pid){
	t_primerNivel* unaTablaDe1erNivel = malloc(sizeof(t_primerNivel));

	for(int i=0;i < list_size(listaT1nivel);i++){

		unaTablaDe1erNivel = list_get(listaT1nivel,i);

		if(unaTablaDe1erNivel->pid == pid){
			return i;
		}
	}
	return 4;//Puse 4 para que no me confunda si retorno 0
	free(unaTablaDe1erNivel);
}


int conexionConCpu(void){


	int server_fd = iniciar_servidor();
	log_info(logger, "Memoria lista para recibir a Cpu o Kernel");
	int cliente_fd = esperar_cliente(server_fd);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			//recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);// esto tengo que usar en cpu para recibir
			log_info(logger, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(logger, "Se desconecto el cliente. Terminando conexion");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;
}

void iterator(char* value) {
	log_info(logger,"%s", value);
}


