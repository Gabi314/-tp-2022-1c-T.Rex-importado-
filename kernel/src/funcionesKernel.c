#include "funcionesKernel.h"

//----------------------------- Para ser servidor de consola ------------------------------------
int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP_KERNEL, PUERTO_KERNEL, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar a consola");

	return socket_servidor;
}

t_config* iniciar_config(void){
	return config_create("kernel.config");
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL); // acepto al cliente------------ ver esto----------------
	log_info(logger, "Se conecto la consola!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente) //No creo que haga falta (estoy de acuerdo! atte: Gabi)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	t_list* listaDeInstrucciones = list_create();
	t_list* identificadores = list_create();

	int tamanioIdentificador;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size)
	{
		instrucciones* instruccion = malloc(sizeof(instrucciones));
		memcpy(&tamanioIdentificador, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		instruccion->identificador = malloc(tamanioIdentificador);
		memcpy(instruccion->identificador, buffer+desplazamiento, tamanioIdentificador);
		desplazamiento+=tamanioIdentificador;
		memcpy(instruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
		desplazamiento+=sizeof(int[2]);
		list_add(listaDeInstrucciones, instruccion);// Despues de esto habria que agragarlas al pcb
		//pcb->instrucciones = listaDeInstrucciones

		list_add(identificadores,instruccion->identificador);//Para que loguee los identificadores cuando les llegan de consola
	}
	free(buffer);
	return identificadores;
}


t_pcb* tomar_pcb(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	buffer = recibir_buffer(&size, socket_cliente);
	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones = list_create();
	int contadorInstrucciones;
	int i = 0;
		memcpy(&pcb->idProceso, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->tamanioProceso, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&contadorInstrucciones, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		while(i < contadorInstrucciones){
			instrucciones* instruccion = malloc(sizeof(instrucciones));
			int identificador_length;
			memcpy(&identificador_length, buffer + desplazamiento, sizeof(int));
			desplazamiento+=sizeof(int);
			instruccion->identificador = malloc(identificador_length);
			memcpy(instruccion->identificador, buffer+desplazamiento, identificador_length);
			desplazamiento+=identificador_length;
			memcpy(instruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
			desplazamiento+=sizeof(int[2]);
			list_add(pcb -> instrucciones, instruccion);
			i++;
		}
		memcpy(&pcb->program_counter, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->tabla_paginas, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		memcpy(&pcb->estimacion_rafaga, buffer + desplazamiento, sizeof(float));
		desplazamiento+=sizeof(float);
		memcpy(&pcb->socket_cliente, buffer + desplazamiento, sizeof(int));
	free(buffer);
	return pcb;
}

//----------------------------- Para ser cliente de Cpu o de Memoria -------------------------------------------------

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, int unPuerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char puerto[20];
	sprintf(puerto,"%d",unPuerto); // convierte el entero a string para el getaddrinfo

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}


void agregar_instrucciones_al_paquete(instrucciones* instruccion) {
	void* id = instruccion->identificador;
	int longitudId = strlen(instruccion->identificador)+1;
	memcpy(paquete->buffer->stream + desplazamiento, &longitudId, sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, id, longitudId);
	desplazamiento+=longitudId;
	memcpy(paquete->buffer->stream + desplazamiento, &(instruccion->parametros), sizeof(int[2]));
	desplazamiento+=sizeof(int[2]);
	free(instruccion->identificador);
	free(instruccion);
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}
/*
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}
*/

void agregar_a_paquete_kernel_cpu(t_pcb* pcb)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	desplazamiento = 0;
	paquete = crear_paquete();
	list_iterate(pcb->instrucciones, (void*) obtenerTamanioIdentificadores);
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanioTotalIdentificadores + contadorInstrucciones*sizeof(int[2]) + contadorInstrucciones*sizeof(int) + 6*sizeof(int) + sizeof(float));
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->idProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->tamanioProceso), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &contadorInstrucciones, sizeof(int));
	desplazamiento+=sizeof(int);
	list_iterate(pcb->instrucciones, (void*) agregar_instrucciones_al_paquete);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->program_counter), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->tabla_paginas), sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->estimacion_rafaga), sizeof(float));
	desplazamiento+=sizeof(float);
	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->socket_cliente), sizeof(int));
	desplazamiento+=sizeof(int);
	paquete->buffer->size = desplazamiento;
	free(pcb);
}



void obtenerTamanioIdentificadores(instrucciones* instruccion) {
	tamanioTotalIdentificadores += (strlen(instruccion -> identificador)+1);
	contadorInstrucciones++;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void eliminar_paquete_mensaje(t_paquete* paqueteMensaje)
{
	free(paqueteMensaje->buffer->stream);
	free(paqueteMensaje->buffer);
	free(paqueteMensaje);
}


void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}



void destruirProceso(t_pcb* proceso){
    close(proceso->socket_cliente);
    free(proceso);
}


//-----------------PLANIFICADOR---------------




t_algoritmo_planificacion obtener_algoritmo(){

	 t_algoritmo_planificacion switcher;
	 char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

	    //FIFO
	    if (strcmp(algoritmo,"FIFO") == 0)
	    {
	        switcher = FIFO;
	        log_info(logger, "El algoritmo de planificacion elegido es FIFO.");
	    }

	    //SFJ CON DESALOJO
	    if (strcmp(algoritmo,"SRT") == 0)
	    {
	        switcher = SRT;
	        log_info(logger, "El algoritmo de planificacion elegido es SJF.");
	    }
	    return switcher;
}// No se usa al final



t_pcb* obtenerSiguienteDeReady(){

	//sem_wait(&contadorReady);

	t_pcb* procesoPlanificado = NULL;

	int tamanioDeColaReady(){

		int tamanio;

		//pthread_mutex_lock(&mutexReady);
		tamanio = list_size(colaReady);
		//pthread_mutex_unlock(&mutexReady);

		return tamanio;
	}

	if (tamanioDeColaReady() > 0){

	// Aca dentro un SWITCH para los distintos algoritmos q llama a una funcion para cada uno
	switch(algoritmoPlanificacionActual){

		//CASO FIFO
		case FIFO:
			procesoPlanificado = obtenerSiguienteFIFO();
		break;

		//CASO SJF con desalojo
		case SRT:
			procesoPlanificado = obtenerSiguienteSRT();
		break;

		}
	}

	// Devuelve NULL si no hay nada en ready
	// Caso contrario devuelve el que tiene mas prioridad segun el algoritmo que se este empleando
	return procesoPlanificado;
}


t_pcb* obtenerSiguienteFIFO(){
	t_pcb* procesoPlanificado = list_get(colaReady,0);
	return procesoPlanificado;
}

t_pcb* obtenerSiguienteSRT(){

	t_pcb* procesoPlanificado = NULL;
	t_pcb* procesoAux = NULL;
	int i;
	int indexARemover;
	float shortestJob;

	//pthread_mutex_lock(&mutexReady);
	procesoAux = list_get(colaReady,0);
	//pthread_mutex_unlock(&mutexReady);

	indexARemover = 0;
	shortestJob = procesoAux->estimacion_rafaga;

	//itero por la lista de Ready
	//sem_wait(&contadorReady);
	//pthread_mutex_lock(&mutexReady);

	log_info(logger,"PROCESOS EN READY: %d \n", list_size(colaReady));


	for(i=1;i<list_size(colaReady);i++){
	   	procesoAux = list_get(colaReady,i);
		if(shortestJob > procesoAux->estimacion_rafaga){
			shortestJob = procesoAux->estimacion_rafaga;
			indexARemover = i;
		}
	}
	procesoPlanificado = list_remove(colaReady, indexARemover);

	//pthread_mutex_unlock(&mutexReady);

	return procesoPlanificado;
}

void estimarRafaga(t_pcb* proceso){
	proceso->estimacion_anterior=proceso->estimacion_rafaga;
	proceso->estimacion_rafaga = alfa * proceso->rafagaMs + (1 - alfa) * proceso->estimacion_rafaga;

}


void enviarInterrupcionACpu(){
	//falta desarrollar
}


void enviarMensajeAMemoria(char* mensaje){
// falta desarrollar
}
void enviarProcesoAMemoria(t_pcb* proceso){
	// falta desarrollar
}
int obtenerValorDeTP(){
	return 1;
	// falta desarrollar
}

t_pcb * recibirProcesoAFinalizar(){
	t_pcb* pcb;
	return pcb;
	// falta desarrollar
}

void enviarMensajeAConsola(char* mensaje){
	// falta desarrollar
}



  //--------------------TRANSICIONES---------------

void agregarANew(t_pcb* proceso) {

	//pthread_mutex_lock(&mutexNew);

	queue_push(colaNew, proceso);
	log_info(logger, "[NEW] Entra el proceso de PID: %d a la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexNew);

	//sem_post(&analizarSuspension); // Despierta al planificador de mediano plazo
	//sem_wait(&suspensionFinalizada); // Espera a que ya se haya hecho, o no, la suspension

	//sem_post(&contadorNew); // Despierta al planificador de largo plazo
	//sem_post(&largoPlazo);
}

t_pcb* sacarDeNew(){

	//sem_wait(&contadorNew);
	//pthread_mutex_lock(&mutexNew);

	t_pcb* proceso = queue_pop(colaNew);
	log_info(logger, "[NEW] Se saca el proceso de PID: %d de la cola", proceso->idProceso);

	//pthread_mutex_unlock(&mutexNew);

	return proceso;
}


void agregarAReady(t_pcb* proceso){

	//sem_wait(&multiprogramacion); Lo sacamos de aca para usarlo en el contexto en el que se llama a la funcion, porque no siempre que se agrega a ready, se toca la multiprogramacion
	//pthread_mutex_lock(&mutexReady);

	if (gradoMultiprogramacionActual <= gradoMultiprogramacionTotal){

		switch(algoritmoPlanificacionActual){

			//CASO FIFO
			case FIFO:
				list_add(colaReady, proceso);
				log_info(logger, "[READY] Entra el proceso de PID: %d a la cola.", proceso->idProceso);
				gradoMultiprogramacionActual ++;
				break;

			//CASO SJF con desalojo
			case SRT:
				//mandar interrupcion a cpu a traves de la conexxion interrupt para indicar que debe desalojar el proceso que esta ejecutando
				list_add(colaReady, proceso);
				log_info(logger, "[READY] Entra el proceso de PID: %d a la cola.", proceso->idProceso);
				gradoMultiprogramacionActual ++;
				break;

				//Aca hay que mandar un mensaje a memoria para que inicialice sus estructuras
			}
	}else{
		log_info(logger, "No se pueden agregar más procesos a READY  se alcanzó el grado de multiprogramácion maximo");
	}
	//pthread_mutex_unlock(&mutexReady);
	//sem_post(&contadorReady);
	//sem_post(&contadorProcesosEnMemoria); Lo sacamos de aca para usarlo en el contexto en el que se llama a la funcion, porque no siempre que se agrega a ready, se toca la multiprogramacion
}


void agregarABlocked(t_pcb* proceso){		//ver semaforos

	//sem_wait(&contadorExe);

	bool tienenMismoPID(void* elemento){

		if(proceso->idProceso == ((t_pcb *) elemento)->idProceso)
		    return true;
		else
			return false;
	}

	list_remove_by_condition(colaExe, tienenMismoPID);

	//pthread_mutex_lock(&mutexBlock);

	list_add(colaBlocked, proceso);
	log_info(logger, "[BLOCK] Entra el proceso de PID: %d a la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexBlock);
	//sem_post(&multiprocesamiento);
	//sem_post(&contadorBlock);

	//sem_post(&analizarSuspension);
	//sem_wait(&suspensionFinalizada);
}

void sacarDeBlocked(t_pcb* proceso){

	//sem_wait(&contadorBlock);

	bool tienenMismoPID(void* elemento){

		if(proceso->idProceso == ((t_pcb *) elemento)->idProceso)
			return true;
		else
			return false;
		}

		//pthread_mutex_lock(&mutexBlock);

		list_remove_by_condition(colaBlocked, tienenMismoPID);
		log_info(logger, "[BLOCK] Sale el proceso de PID: %d de la cola.", proceso->idProceso);

		//pthread_mutex_unlock(&mutexBlock);
}




void agregarASuspendedBlocked(t_pcb* proceso){

	//pthread_mutex_lock(&mutexBlockSuspended);

	/*proceso->suspendido = true;*/ proceso->estado = SUSP_BLOCKED;

	list_add(colaSuspendedBlocked, proceso);

	log_info(logger, "[SUSPENDEDBLOCKED] Ingresa el proceso de PID: %d a la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexBlockSuspended);

	/*size_t size = sizeof(sem_code)+sizeof(uint32_t);

	void* stream = malloc(size);

	sem_code opCode = SUSPEND;

	memcpy(stream, &opCode, sizeof(sem_code));
	memcpy(stream + sizeof(sem_code), &(proceso->idProceso), sizeof(uint32_t));

	send(proceso->socketMemoria, stream, size, 0);

	free(stream);
	*/  //Aca tenemos que mandar el proceso a disco
}

void sacarDeSuspendedBlocked(t_pcb* proceso){

	bool tienenMismoPID(void* elemento){

	if(proceso->idProceso == ((t_pcb *) elemento)->idProceso)
		return true;
	else
		return false;
	}

	//pthread_mutex_lock(&mutexBlockSuspended);

	list_remove_by_condition(colaSuspendedBlocked, tienenMismoPID);
	log_info(logger, "[SUSPENDEDBLOCKED] Sale el proceso de PID: %d de la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexBlockSuspended);
}

void agregarAReadySuspended(t_pcb* proceso){

	//pthread_mutex_lock(&mutexReadySuspended);

	queue_push(colaSuspendedReady, proceso);
	log_info(logger, "[SUSPENDEDREADY] Ingresa el proceso de PID: %d de la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexReadySuspended);

	//sem_post(&contadorReadySuspended);
	//sem_post(&medianoPlazo);
}

t_pcb* sacarDeSuspendedReady(){

	//sem_wait(&contadorReadySuspended);

	//pthread_mutex_lock(&mutexReadySuspended);

	t_pcb* proceso = queue_pop(colaSuspendedReady);
	/*proceso->suspendido = false;*/
	log_info(logger, "[SUSPENDEDREADY] Sale el proceso de PID: %d de la cola.", proceso->idProceso);

	//pthread_mutex_unlock(&mutexReadySuspended);

	return proceso;
}



void ejecutar(t_pcb* proceso){


	time_t a = time(NULL);
	proceso->horaDeIngresoAExe = ((float) a)*1000;

	send(socketCpuDispatch, proceso, proceso->tamanioProceso, 0);

	//Aca mando el proceso a cpu y guardo el momento en el que arranca a ejecutar para el calculo de la estimacion
}

// Las funciones siguientes son hilos del planificador

void newAReady(){

	while(1){

		//sem_wait(&largoPlazo);

		if(queue_size(colaSuspendedReady) != 0){

			//sem_post(&medianoPlazo);
		}else{

			t_pcb* proceso = sacarDeNew();

			//carpincho->rafagaAnterior = 0;
			proceso->estimacion_anterior = estimacionInicial;
			proceso->estimacion_rafaga = estimacionInicial;	//"estimacionInicial" va a ser una variable que vamos a obtener del cfg

			//sem_wait(&multiprogramacion);
			agregarAReady(proceso);
			//sem_post(&contadorProcesosEnMemoria);
		}
	} //aca hay que replanificar en caso de SRT?
}

void readyAExe(){

	while(1){

		t_pcb* procesoAEjecutar = obtenerSiguienteDeReady();


		if(procesoAEjecutar != NULL) {

			//pthread_mutex_lock(&mutexExe);
			ejecutar(procesoAEjecutar);
			//pthread_mutex_unlock(&mutexExe);

			if(algoritmoPlanificacion == SRT){
				log_info(logger, "[EXEC] Ingresa el proceso de PID: %d con una rafaga de ejecucion estimada de %f milisegundos.", procesoAEjecutar->idProceso, procesoAEjecutar->estimacion_rafaga);
			}else{
				log_info(logger, "[EXEC] Ingresa el proceso de PID: %d", procesoAEjecutar->idProceso);
			} //ver bien para el caso FIFO


			//sem_post(&analizarSuspension); // Despues de que un proceso se va de Ready y hace su transicion, se analiza la suspension
			//sem_wait(&suspensionFinalizada);

		}else{
			log_info(logger, "[EXEC] No se logró encontrar un proceso para ejecutar");
		}
	}
}

void blockedASuspension(){

	while(1){

		//sem_wait(&analizarSuspension);


		t_pcb* procesoASuspender = list_find(colaBlocked,supera_tiempo_maximo_bloqueado);

		if(procesoASuspender != NULL){

			//sem_wait(&contadorProcesosEnMemoria);

			sacarDeBlocked(procesoASuspender);

			agregarASuspendedBlocked(procesoASuspender);

			usleep((tiempoMaximoBloqueado+(obtenerTiempoDeBloqueo(procesoASuspender) - tiempoMaximoBloqueado))/1000); //divido por 1000 para pasar a milisegundos

			//sem_post(&multiprogramacion);
		}

		//sem_post(&suspensionFinalizada);
	}
}


void suspensionAReady(){

	while(1){

		//sem_wait(&medianoPlazo);

		if(queue_size(colaSuspendedReady) == 0){

			//sem_post(&largoPlazo);
		}else{

		t_pcb* proceso = sacarDeSuspendedReady();

		//sem_wait(&multiprogramacion);

		agregarAReady(proceso);

		//sem_post(&contadorProcesosEnMemoria);
		}
	}
}




int obtenerTiempoDeBloqueo(t_pcb* proceso){
	instrucciones* primeraInstruccion;
	primeraInstruccion = list_get(proceso->instrucciones,0);
	return primeraInstruccion->parametros[0];

}

bool supera_tiempo_maximo_bloqueado(t_pcb* proceso){
	if (obtenerTiempoDeBloqueo(proceso) > tiempoMaximoBloqueado)
		return true;
	else
		return false;

}




void terminarEjecucion(t_pcb* proceso){

	//pthread_mutex_lock(&mutexExit);

	list_add(colaExit, proceso);
	log_info(logger, "[EXIT] Finaliza el proceso de PID: %d", proceso->idProceso);

	//pthread_mutex_unlock(&mutexExit);


	//Aca hay que mandar un mensaje a memoria para que libere sus estructuras
	//Y tambien avisar a consola que termino su ejecucion
}



//------------------------HILOS-------------------------------


// * MULTIHILOS DE EJECUCION PARA ATENDER N CONSOLAS: esperan a recibir una consola y sus
// instrucciones para generar el pcb y asignar el proceso a NEW

void  recibir_consola(int servidor) {

		while(1) {
		pthread_t hilo1;

		int nuevo_cliente = esperar_cliente(servidor);
		int hiloCreado = pthread_create(&hilo1, NULL,&atender_consola,nuevo_cliente);

		pthread_detach(hiloCreado);
		}
		//int ultimo_cliente = nuevo_cliente; ---> en caso de que repita siempre la misma consola habria que hacer algo asi (creo)

	}

	void atender_consola(int nuevo_cliente) {
			t_pcb* PCB;
			t_list* listaDeInstrucciones = recibir_paquete(nuevo_cliente);
			// no las devuelve pero la idea es que lo haga o  en su defecto crear una funcion que lo haga
			t_list* proximaInstruccion = listaDeInstrucciones;

			PCB->idProceso = 1000; // esto después va a ser un numero random por cada consola conectada
			PCB->tamanioProceso = 512; // idem
			PCB->instrucciones = listaDeInstrucciones;
			PCB->program_counter = proximaInstruccion;
			PCB->estimacion_rafaga= estimacionInicial;
			PCB->estado = NEW;
			PCB->socket_cliente = nuevo_cliente; // acá guardamos el socket
			PCB->tiempoEjecucionRealInicial = 0 ;
			PCB->tiempoEjecucionAcumulado = 0;


			agregarANew(PCB);

		}

	/*
	ASIGNAR_MEMORIA(): si el grado de multiprogramacion lo permite, pasa el primer proceso de colaNew a READY, envia
	un mensaje al módulo Memoria para que inicialice sus estructuras necesarias y obtener el valor de la tabla de páginas
	del PCB. Además, si el algoritmo es SRT envía una interrupcion a cpu.
	 */
	void asignar_memoria() {

		while(1){

		t_pcb* proceso = sacarDeNew();
		agregarAReady(proceso);
		enviarMensajeAMemoria("Inicializar estructuras"); // falta desarrollar
		enviarProcesoAMemoria(proceso); // falta desarrollar
		int NroTP = obtenerValorDeTP();  // falta desarrollar

		if(algoritmoPlanificacion == SRT)
			enviarInterrupcionACpu(); // falta desarrollar
		}
	}

	/*
	  * FINALIZAR_PROCESO_Y_AVISAR_A_MEMORIA(): Recibe un PCB con motivo de finalizar el mismo, pasa al
		proceso al estado EXIT y da aviso al módulo Memoria para que éste libere sus estructuras. La idea sería tener
	  	un semaforo o algo que controle que la ejecucion del proceso sea la última
	 */

	void finalizar_proceso_y_avisar_a_memoria() {
		while(1){
		t_pcb * proceso = recibirProcesoAFinalizar(); //falta desarrollar
		terminarEjecucion(proceso);

		enviarMensajeAMemoria("Liberar estructuras"); // falta desarrollar
		}
	}

	/*
	 * FINALIZAR_PROCESO_Y_AVISAR_A_CONSOLA(): Una vez liberadas las estructuras de CPU, se dará aviso a la Consola
		de la finalización del proceso. La idea seria que espere el pcb de cpu con un mensaje que avise la liberación
		de las estructuras
	 */

	void finalizar_proceso_y_avisar_a_consola() {
		while(1){
		enviarMensajeAConsola("Fin del proceso"); // falta desarrollar
		}

	}

	//* PLANIFICAR(): Llama constantemente al planificador.

	void planificar() {
		while(1) {
			readyAExe(); //en principio llama a esta transicion ya que es la única que invoca a los planificadores y además se encuentra aislada del resto
		}				// igual ojo porque parece que falta actualizarla
	}

	/*
	 * SUSPENDER(): si un proceso está bloqueado por un tiempo mayor al límite se llamará a una transición para
		           suspenderlo y se enviará un mensaje a memoria con la informacion necesaria.
	 */

	void suspender(){
		while(1){
		blockedASuspension(); // modificar: la idea sería que solo tome el primer elemento de la lista y lo pase a bloqueado y suspendido
		}
	}

	/*
	 * DESBLOQUEAR_SUSPENDIDO(): espera a que termine la entrada/salida de un proceso SUSPENDED-BLOCKED y llama
		a las transiciones necesarias para que pase a ser SUSPENDED-READY
	 */

	void desbloquear_suspendido(){
		while(1){
		t_pcb * proceso = queue_pop(colaSuspendedBlocked);
		agregarAReadySuspended(proceso);
		}
	}

