#include "funcionesMemoria.h"

int marcosTotales = 0;
//Antes de esto debo mandarle a cpu tam_Pagina y cant_entradas_por_pagina
int pid = 0;//Viene de Kernel, para devolver nroDeTabla1erNivel
//int entradaDeTablaDe1erNivel = 2;  Viene de cpu(calculado con la dir logica), con esto devuelvo nroTablaDe2doNivel

int contPaginas = 0;
int contTablas2doNivel = 0;
int flagDeEntradasPorTabla = 0;
int contadorDeMarcosPorProceso = 0;


t_list* listaT1nivel;
t_list* listaDeMarcos;
t_list* listaT2Nivel;
t_list* listaDePaginasEnMemoria;
t_list* listaDePaginas;


int main(void) {
	logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
	crearConfiguraciones();

	listaT1nivel = list_create();
	listaDeMarcos = list_create();
	listaT2Nivel = list_create();
	listaDePaginasEnMemoria = list_create();
	listaDePaginas = list_create();


	pagina* unaPagina1 = malloc(sizeof(pagina));
	unaPagina1->numeroDePagina = 1;
	unaPagina1->uso = 1;
	unaPagina1->numeroMarco = 123;
	unaPagina1->modificado = 1;
	pagina* unaPagina2 = malloc(sizeof(pagina));
	unaPagina2->numeroDePagina = 2;
	unaPagina2->uso = 1;
	unaPagina2->numeroMarco = 100;
	unaPagina2->modificado = 1;
	pagina* unaPagina3 = malloc(sizeof(pagina));
	unaPagina3->numeroDePagina = 3;
	unaPagina3->uso = 1;
	unaPagina3->numeroMarco = 142;
	unaPagina3->modificado = 1;
	pagina* unaPagina4 = malloc(sizeof(pagina));
	unaPagina4->numeroDePagina = 4;
	unaPagina4->uso = 1;
	unaPagina4->numeroMarco = 36514;
	unaPagina4->modificado = 1;

	list_add(listaDePaginas,unaPagina1);
	list_add(listaDePaginas,unaPagina2);
	list_add(listaDePaginas,unaPagina3);
	list_add(listaDePaginas,unaPagina4);


	inicializarEstructuras();

	//conexionConCpu();

	inicializarMarcos();

	int nroDeMarco = algoritmoClockM(listaDePaginas);

	log_info(logger,"nro de marco %d", nroDeMarco);
	//log_info(logger,"nro de marco %d",25);


}

int conexionConCpu(void){


	int server_fd = iniciar_servidor();
	log_info(logger, "Memoria lista para recibir a Cpu o Kernel");
	int cliente_fd = esperar_cliente(server_fd);

	nroTabla1erNivelYentrada = list_create();

	int a = 1;
	while (a == 1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(cliente_fd);//recibe el pedido de tam_pag y cant_entradas
				enviarTamanioDePaginaYCantidadDeEntradas(cliente_fd);
				break;
			case PAQUETE:
				nroTabla1erNivelYentrada = recibir_nroTabla1erNivel_entradaTabla1erNivel(cliente_fd); // lista con valores de nro y entrada de tabla

				int nroTabla2doNivel = leerYRetornarNroTabla2doNivel(nroTabla1erNivelYentrada);

				enviarNroTabla2doNivel(cliente_fd,nroTabla2doNivel);
				a = 0;
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

int leerYRetornarNroTabla2doNivel(t_list* nroTabla1erNivelYentrada){
	nroTablaDePaginas1erNivel = (int) list_get(nroTabla1erNivelYentrada,0);
	entradaTablaDePaginas1erNivel = (int) list_get(nroTabla1erNivelYentrada,1);

	log_info(logger,"Me llego el nroTabla1erNivel %d y con la entrada %d",nroTablaDePaginas1erNivel,entradaTablaDePaginas1erNivel);

	return numeroTabla2doNivelSegunIndice(nroTablaDePaginas1erNivel,entradaTablaDePaginas1erNivel);
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
		tablaDeSegundoNivel->numeroTabla = contTablas2doNivel;
		contTablas2doNivel++; //Contador global que asigna el numero de tabla de 2do nivel a nivel global, igual al indice en la lista

		cargarPaginas(tablaDeSegundoNivel);
		//tablaDeSegundoNivel->marcos = contadorDeMarcos;

		list_add(tablaPrimerNivel->tablasDeSegundoNivel, tablaDeSegundoNivel);
		list_add(listaT2Nivel,tablaDeSegundoNivel);


		//free(tablaDeSegundoNivel);
	}

	contPaginas = 0;


	list_add(listaT1nivel,tablaPrimerNivel);

	//free(tablaPrimerNivel);

	escribirEnSwap(pid);
}

void inicializarMarcos(){

	for(int j=0;j<(tamanioDeMemoria/tamanioDePagina);j++){
		marco* unMarco = malloc(sizeof(marco));
		unMarco->numeroDeMarco = j;
		unMarco->marcoLibre = 0;
		list_add(listaDeMarcos,unMarco);

		//free(unMarco);

	}
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

//Funcion para obtener el primer marco libre
int siguienteMarcoLibre(){
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0;i < list_size(listaDeMarcos);i++){

		unMarco = list_get(listaDeMarcos,i);

		if(unMarco->marcoLibre == 0){
			return unMarco->numeroDeMarco;
		}
	}
}


int busquedaDePaginaConUsoCero(t_list* listaDePaginas){

	return 0;
}

void reemplazarTodosLosUsoACero(t_list* unaListaDePaginas){
	pagina* unaPagina = malloc(sizeof(pagina));

	for(int i = 0; i<list_size(unaListaDePaginas);i++){
					unaPagina = list_get(unaListaDePaginas,i);
					unaPagina->uso = 0;
				}
}





int algoritmoClock(t_list* unaListaDePaginas){
	pagina* unaPagina = malloc(sizeof(pagina));

	for(int i = 0; i<list_size(unaListaDePaginas);i++){
	unaPagina = list_get(unaListaDePaginas,i);

		if(unaPagina->uso == 0){
				int numeroDeMarcoAReemplazar = unaPagina->numeroMarco;
				sacarMarcoAPagina(unaPagina);
				return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso en 0");
	log_info(logger,"Por algoritmo reemplazo todos los bit de uso a 0 y busco de nuevo");
	reemplazarTodosLosUsoACero(unaListaDePaginas);

	for(int i = 0; i<list_size(unaListaDePaginas);i++){
	unaPagina = list_get(unaListaDePaginas,i);

		if(unaPagina->uso == 0){
				int numeroDeMarcoAReemplazar = unaPagina->numeroMarco;
				sacarMarcoAPagina(unaPagina);
				return numeroDeMarcoAReemplazar;
		}
	}

}
int algoritmoClockM (t_list* unaListaDePaginas){
	pagina* unaPagina = malloc(sizeof(pagina));

	for(int i = 0; i<list_size(unaListaDePaginas);i++){
		unaPagina = list_get(unaListaDePaginas,i);

		if(unaPagina->uso == 0 && unaPagina->modificado == 0){

			int numeroDeMarcoAReemplazar = unaPagina->numeroMarco;
			sacarMarcoAPagina(unaPagina);
			return numeroDeMarcoAReemplazar;

		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(unaListaDePaginas);i++){
		unaPagina = list_get(unaListaDePaginas,i);

		if(unaPagina->uso == 0 && unaPagina->modificado == 1){

			int numeroDeMarcoAReemplazar = unaPagina->numeroMarco;
			sacarMarcoAPagina(unaPagina);
			return numeroDeMarcoAReemplazar;
		}
	}

	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 1");
	log_info(logger,"Reemplazo todos los bit de uso en 0");
	reemplazarTodosLosUsoACero(unaListaDePaginas);
	log_info(logger,"Busco con bit de uso en 0 y modificado en 0 ");

	for(int i = 0; i<list_size(unaListaDePaginas);i++){
		unaPagina = list_get(unaListaDePaginas,i);

		if(unaPagina->uso == 0 && unaPagina->modificado == 0){

			int numeroDeMarcoAReemplazar = unaPagina->numeroMarco;
			sacarMarcoAPagina(unaPagina);
			return numeroDeMarcoAReemplazar;
		}
	}
	log_info(logger,"No hay ninguna pagina con bit de uso en 0 y modificado en 0");
	log_info(logger,"Busco con bit de uso en 0 y modificado en 1");

	for(int i = 0; i<list_size(unaListaDePaginas);i++){
		unaPagina = list_get(unaListaDePaginas,i);

		if(unaPagina->uso == 0 && unaPagina->modificado == 1){

			int numeroDeMarcoAReemplazar = unaPagina->numeroMarco;
			sacarMarcoAPagina(unaPagina);
			return numeroDeMarcoAReemplazar;
			}
			}
}

marco* buscarMarco(int nroDeMarco){
	marco* unMarco = malloc(sizeof(marco));

	for(int i=0;i < list_size(listaDeMarcos);i++){

		unMarco = list_get(listaDeMarcos,i);

		if(unMarco->numeroDeMarco == nroDeMarco){
			return unMarco;
		}
	}
	free(unMarco);
}

//Funcion de cargar una pagina, por ahora la hago global y en 1 proceso porque no se como funciona
void cargarPagina(pagina* unaPagina){
	marco* marcoAAsignar;
	//Caso en el que se puede asignar un marco a un proceso de manera libre
	if(contadorDeMarcosPorProceso<marcosPorProceso){
		marcoAAsignar = siguienteMarcoLibre();
		modificarPaginaACargar(unaPagina,marcoAAsignar->numeroDeMarco);
		list_add(listaDePaginasEnMemoria,unaPagina);
		contadorDeMarcosPorProceso++;

	}//Caso en el que ya el proceso tiene maxima cantidad de marcos por proceso y hay que desalojar 1

	else{
		if(algoritmoDeReemplazo == "CLOCK"){
			int marcoAAsignar = algoritmoClock(listaDePaginasEnMemoria);
			modificarPaginaACargar(unaPagina,marcoAAsignar);
			list_add(listaDePaginasEnMemoria,unaPagina);
	}else{
		if(algoritmoDeReemplazo == "CLOCK M"){
			int marcoAAsignar = algoritmoClockM(listaDePaginasEnMemoria);
			modificarPaginaACargar(unaPagina,marcoAAsignar);
			list_add(listaDePaginasEnMemoria,unaPagina);

	}
	}
	}
}

void modificarPaginaACargar(pagina* unaPagina, int nroDeMarcoAASignar){
	unaPagina->numeroMarco = nroDeMarcoAASignar;
	unaPagina->presencia = 1;
	unaPagina->uso = 1;
	//unaPagina->modificado = 0; ANALIZAR CASO DE MODIFICADO EN 1
	marco* marcoAsignado = buscarMarco(nroDeMarcoAASignar);
	marcoAsignado->marcoLibre = 1;
}

void sacarMarcoAPagina(pagina* unaPagina){
	unaPagina->presencia = 0;
	unaPagina->uso = 0;
	unaPagina->numeroMarco = -1;

}

//Funcion para chequear cantidad de marcos. CREO QUE NO ES UTIL PORQUE LO HAGO EN EL IF DE CARGAR PAGINA
void chequeoCantidadDeMarcosPorProceso(){

}

//Funcion para reemplazar una pagina, tener en cuenta info de la pagina
void reemplazoDePagina(){

}




/*Entra proceso, inicializa y devuelvo tabla de primer nivel, cpu pide pagina se chequea si esta cargada y sino voy a
 *ir asignanco marcos, cambio bit de presencia, hasta el maximo definido por archivo config
 *
 *Cpu viene con tabla de 1er nivel y con la entrada de tabla de primer nivel, le devuelvo tabla de 2do nivel
 *Cpu viene con tabla de 2do nivel y con la entrada de tabla de 2do nivel, le devuelvo el marco si esta cargado sino, cargo y etc
 *
 *Estructura de memoria, con todos los marcos y los valores que les vamos escribiendo.
 */

void chequeoDeIndice(int indice){
	if(indice<entradasPorTabla){
		flagDeEntradasPorTabla = 1;
	}
}


int numeroTabla2doNivelSegunIndice(int numeroTabla1erNivel,int indiceDeEntrada){
	chequeoDeIndice(indiceDeEntrada);

	if(flagDeEntradasPorTabla == 1){
		t_primerNivel* unaTablaDe1erNivel = malloc(sizeof(t_primerNivel));
		t_segundoNivel* unaTablaDe2doNivel = malloc(sizeof(t_segundoNivel));
		unaTablaDe1erNivel = list_get(listaT1nivel,numeroTabla1erNivel);
		unaTablaDe2doNivel = list_get(unaTablaDe1erNivel->tablasDeSegundoNivel,indiceDeEntrada);
		flagDeEntradasPorTabla = 0;

		free(unaTablaDe1erNivel);
		return unaTablaDe2doNivel->numeroTabla; //El indice de entrada es igual al numero de tabla de 2do nivel
	}else{
		return -1;//Si da -1 printeo un error
	}

}
//numeroTabla1erNivel es igual al indice en la lista de tablas de primer nivel
//Ahora tengo que agarrar la lista de tablas de 2do, buscar el primero que tenga el pid de la tablaDe1erNive
//A ese indice sumarla el indiceDeEntrada y esa es la tablaDe2doNivel que necesito
//Si el indice = 2, en la lista de tablas de 2do nivel, agarro posicion 2, que es la tabla numero 2, entonces es redundante




//contador de marcos por proceso, asigno sumo 1, hasta maximo de marcos por procesos y ahi empiezo a reemplazar

//Funcion de cargar pagina en memoria



int marcoSegunIndice(int numeroTabla2doNivel,int indiceDeEntrada){

	pagina* unaPagina = malloc(sizeof(pagina));
	marco* unMarco = malloc(sizeof(marco));
	t_segundoNivel* unaTablaDe2doNivel = malloc(sizeof(t_segundoNivel));

	chequeoDeIndice(indiceDeEntrada);

	if(flagDeEntradasPorTabla == 1){
	int posicionDeLaTablaBuscada = buscarNroTablaDe2doNivel(numeroTabla2doNivel);
	unaTablaDe2doNivel = list_get(listaT2Nivel,posicionDeLaTablaBuscada);

	unaPagina = list_get(unaTablaDe2doNivel->paginas,indiceDeEntrada);

	flagDeEntradasPorTabla = 0;

		if(unaPagina->presencia == 1){
			return unaPagina->numeroMarco;
	}
		else{
	//En el caso en que no este cargada en memoria, tengo que cargarla asignando numero de marco y cambiando bit de presencia a 1
	//Tengo que analizar caso en que no haya paginas por cargar, tengo que hacer contador por proceso
	//ver tema de si es una pagina con info para swap

		}
	}

}

int buscarNroTablaDe2doNivel(int numeroDeTabla2doNivel){
	t_segundoNivel* unaTablaDe2doNivel = malloc(sizeof(t_segundoNivel));

	for(int i=0;i < list_size(listaT2Nivel);i++){

		unaTablaDe2doNivel = list_get(listaT2Nivel,i);

		if(unaTablaDe2doNivel->numeroTabla == numeroDeTabla2doNivel){
			return i;
		}
	}
	// return 4;//Puse 4 para que no me confunda si retorno 0
	free(unaTablaDe2doNivel);
}

void leerElPedido(int posicion){

}

void escribirElPedido(char* loQueSeEscribe,int posicion){

}



void iterator(char* value) {
	log_info(logger,"%s", value);
}

// falta terminar programa


