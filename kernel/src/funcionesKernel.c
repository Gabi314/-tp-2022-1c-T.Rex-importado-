#include "funcionesKernel.h"

int min (int x, int y)
{
	if (x>y) {
		return (y);
	}else{
		return (x);
	}
}

//----------------------------- Para ser servidor de consola ------------------------------------
int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

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

void recibir_mensaje(int socket_cliente) //No creo que haga falta
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete_int(int socket_cliente){
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size){
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		int valor = 0;
		memcpy(&valor, buffer+desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		list_add(valores, (void *)valor);
	}

	free(buffer);
	return valores;
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;

	t_list* listaDeInstrucciones = list_create();

	int tamanioIdentificador;

	buffer = recibir_buffer(&size, socket_cliente);

	while(desplazamiento < size)
	{
		instruccion* unaInstruccion = malloc(sizeof(instruccion));
		memcpy(&tamanioIdentificador, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		unaInstruccion->identificador = malloc(tamanioIdentificador);
		memcpy(unaInstruccion->identificador, buffer+desplazamiento, tamanioIdentificador);
		desplazamiento+=tamanioIdentificador;
		memcpy(unaInstruccion->parametros, buffer+desplazamiento, sizeof(int[2]));
		desplazamiento+=sizeof(int[2]);
		list_add(listaDeInstrucciones, unaInstruccion);// Despues de esto habria que agragarlas al pcb
		//pcb->instrucciones = listaDeInstrucciones


	}
	free(buffer);
	return listaDeInstrucciones;
}

t_list* recibir_paqueteInt(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		int valor = 0;
		memcpy(&valor, buffer+desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		list_add(valores, (void *)valor);
	}
	free(buffer);
	return valores;
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
		instruccion* instruccion = malloc(sizeof(instruccion));
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
	//memcpy(&pcb->socket_cliente, buffer + desplazamiento, sizeof(int));

	//agregar rafagaMs clock_t horaDeIngresoAExe y t_estado estado

	free(buffer);
	return pcb;
}

//----------------------------- Para ser cliente de Cpu o de Memoria -------------------------------------------------

void* serializar_paquete(t_paquete* paquete, int bytes)
{

	void * magic = malloc(bytes);
	int desplazamiento = 0;

	if(socketMemoria){
		memcpy(magic + desplazamiento, &(paquete->codigo_operacion_memoria), sizeof(int));
	}else if(socketCpuDispatch){
		memcpy(magic + desplazamiento, &(paquete->codigo_operacion_cpu), sizeof(int));
	}

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

	char* puerto = string_itoa(unPuerto);  // convierte el entero a string para el getaddrinfo

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente,int cod_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	if(socket_cliente == socketMemoria){
		paquete->codigo_operacion_memoria = cod_op;
	}else if(socket_cliente == socketCpuDispatch){
		paquete->codigo_operacion_cpu = cod_op;
	}

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

t_paquete* crear_paquete(int cod_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	if(socketMemoria){
		paquete->codigo_operacion_memoria = cod_op;
	}else if(socketCpuDispatch){
		paquete->codigo_operacion_cpu = cod_op;
	}
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_a_paquete_kernel_cpu(t_pcb* pcb)
{
	tamanioTotalIdentificadores = 0;
	contadorInstrucciones = 0;
	desplazamiento = 0;
	paquete = crear_paquete(ENVIAR_PCB);
	list_iterate(pcb->instrucciones,(void*) obtenerTamanioIdentificadores);

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
//	memcpy(paquete->buffer->stream + desplazamiento, &(pcb->socket_cliente), sizeof(int));
	//desplazamiento+=sizeof(int);
	paquete->buffer->size = desplazamiento;

	log_info(logger,"Le envio a cpu el pcb");
	enviar_paquete(paquete,socketCpuDispatch);
	eliminar_paquete(paquete);
	
	free(pcb);
}

void obtenerTamanioIdentificadores(instruccion* unaInstruccion) {
	tamanioTotalIdentificadores += strlen(unaInstruccion -> identificador)+1;
	contadorInstrucciones++;
}

void agregar_instrucciones_al_paquete(instruccion* instruccion) {
	char* id = instruccion->identificador;
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

void destruirProceso(t_pcb* proceso) {
	// close(proceso->socket_cliente);
	free(proceso);
}

//-----------------PLANIFICACION Y TRANSICIONES---------------

t_pcb* sacarDeNew() {

	t_pcb* proceso = queue_pop(colaNew);
	log_info(logger, "[NEW] Se saca el proceso de PID: %d de la cola",
			proceso->idProceso);

	return proceso;
}

void agregarABlocked(t_pcb* proceso) {

	queue_push(colaBlocked, proceso);
	log_info(logger, "[BLOCK] Entra el proceso de PID: %d a la cola.",
			proceso->idProceso);

}

void sacarDeBlocked() {
	t_pcb * proceso;

	proceso = list_remove(colaBlocked, 0);
	log_info(logger, "[BLOCK] Sale el proceso de PID: %d de la cola.",
			proceso->idProceso);

	free(proceso);

}

void agregarASuspendedBlocked(t_pcb* proceso) {
	queue_push(proceso,colaSuspendedBlocked);
}

void sacarDeSuspendedBlocked(t_pcb* proceso) {

	bool tienenMismoPID(void* elemento) {

		if (proceso->idProceso == ((t_pcb *) elemento)->idProceso)
			return true;
		else
			return false;
	}

	list_remove_by_condition(colaSuspendedBlocked, tienenMismoPID);
	log_info(logger,
			"[SUSPENDEDBLOCKED] Sale el proceso de PID: %d de la cola.",
			proceso->idProceso);
}

void agregarASuspendedReady(t_pcb* proceso) {

	queue_push(colaSuspendedReady, proceso);
	log_info(logger,
			"[SUSPENDEDREADY] Ingresa el proceso de PID: %d de la cola.",
			proceso->idProceso);

	sem_post(&suspAReady);

}

t_pcb* sacarDeSuspendedReady() {

	//sem_wait(&contadorReadySuspended);

	//pthread_mutex_lock(&mutexReadySuspended);

	t_pcb* proceso = queue_pop(colaSuspendedReady);
//	proceso->suspendido = false;
	log_info(logger, "[SUSPENDEDREADY] Sale el proceso de PID: %d de la cola.",
			proceso->idProceso);

	//pthread_mutex_unlock(&mutexReadySuspended);

	return proceso;
}


int obtenerTiempoDeBloqueo(t_pcb* proceso) {
	instruccion* instruccionActual;
	instruccionActual = list_get(proceso->instrucciones,
			(proceso->program_counter) - 1);
	return instruccionActual->parametros[0];

}


bool supera_tiempo_maximo_bloqueado(t_pcb* proceso) {
	if (obtenerTiempoDeBloqueo(proceso) > tiempoMaximoBloqueado)
		return true;
	else
		return false;

}



//------------------------HILOS-------------------------------

void recibir_consola(int servidor) {

	while (1) {
		pthread_t hilo1;
		log_info(logger,"esperamos una nueva consola");
		int nuevo_cliente = esperar_cliente(servidor);

		log_info(logger,"recibimos al cliente de socket %d", nuevo_cliente);

		int hiloCreado = pthread_create(&hilo1, NULL, &atender_consola,nuevo_cliente);
		pthread_detach(hilo1);

		log_info(logger,"levantamos y detacheamos el hilo de la consola %d", nuevo_cliente);
	}

}

void atender_consola(int nuevo_cliente) {
	t_pcb* PCB = malloc(sizeof(t_pcb));

	conexionConConsola(nuevo_cliente);

	log_info(logger,"[atender_consola]recibimos las instrucciones del cliente %d!", nuevo_cliente);


	PCB->idProceso = 0; //poner un contador
	PCB->tamanioProceso = tamanioDelProceso; // lo recibe de consola

	PCB->instrucciones = list_create();

	PCB->instrucciones = listaInstrucciones;
	PCB->program_counter = 0;

	//int nroTabla1erNivel = conexionConMemoria();

	PCB->tabla_paginas = -1;// por ahora es -1
	PCB->estimacion_rafaga = estimacionInicial;
	PCB->rafagaMs = 0;
	PCB->horaDeIngresoAExe = 0;
	PCB->estado = NEW;

	log_info(logger,"[atender_consola]inicializamos PCB de id_proceso %d", PCB->idProceso);

	t_pidXsocket* datosCliente = malloc(sizeof(t_pidXsocket));
	datosCliente->pid = PCB->idProceso;
	datosCliente->socket = nuevo_cliente;

	log_info(logger,"[atender_consola]creamos la estructura datosCLiente de socket %d y pid %d", nuevo_cliente, PCB->idProceso);
	pthread_mutex_lock(&consolaNueva);
	list_add(listaDeConsolas, datosCliente); // acá guardamos el socket
	pthread_mutex_unlock(&consolaNueva);
	log_info(logger,"[atender_consola]agregamos la estructura a la lista de consolas!");

	pthread_mutex_lock(&encolandoPcb);
	agregarANew(PCB);
	pthread_mutex_unlock(&encolandoPcb);

	log_info(logger,"[atender_consola] despertamos a asignar_memoria()");

	sem_post(&pcbEnNew);
}

void agregarANew(t_pcb* proceso) {

	queue_push(colaNew, proceso);
	log_info(logger, "[NEW] Entra el proceso de PID: %d a la cola.",
			proceso->idProceso);
	log_info(logger,"el tamanio de la cola de new es %d", queue_size(colaNew));
}


void asignar_memoria() {

	while (1) {

		sem_wait(&pcbEnNew);
		sem_wait(&gradoDeMultiprogramacion);
		t_pcb* proceso;
		log_info(logger, "[asignar_memoria] :se desperto ");
		pthread_mutex_lock(&asignarMemoria);

		if (queue_size(colaSuspendedReady) != 0) {
			proceso = sacarDeSuspendedReady();
			agregarAReady(proceso);


		} else {
			proceso = sacarDeNew();

			log_info(logger, "[asignar_memoria]: enviamos el pid %d",proceso->idProceso);

			int nroTabla1erNivel = conexionConMemoria();

			proceso->tabla_paginas = nroTabla1erNivel;

			log_info(logger,"[asignar_memoria]: recibimos el numero de tabla de paginas %d",proceso->tabla_paginas);

			agregarAReady(proceso);

			pthread_mutex_unlock(&asignarMemoria);

		}

		log_info(logger,"[asignar_memoria]: desde asignar_memoria despertamos a readyAExe");
	}

}

void agregarAReady(t_pcb* proceso) {
list_add(colaReady, proceso);

log_info(logger, "[READY] Entra el proceso de PID: %d a la cola.",
		proceso->idProceso);

	if ((algoritmoPlanificacion == SRT) && ejecucionActiva) {
		log_info(logger,
				"[agregarAReady]: desde asignar_memoria despertamos a atenderDesalojo");
		sem_post(&desalojarProceso);
		sem_wait(&procesoDesalojadoSem);
		log_info(logger,
				"[agregarAReady]: desde asignar_memoria sabemos que el proceso fue desalojado");
	}

	log_info(logger,"[agregarAReady]:el tamanio de la cola de ready es %d", list_size(colaReady));

	log_info(logger,"[agregarAReady]: despertamos readyAExe)");

sem_post(&pcbEnReady);
}

void readyAExe() {

//int n;

while (1) {

	sem_wait(&pcbEnReady);
	sem_wait(&cpuDisponible);

	log_info(logger, "[readyAExe]: .........se despierta readyAExe ");
	log_info(logger, "[readyAExe]: a partir de ahora, CPU NO DISPONIBLE! ");

	pthread_mutex_lock(&obtenerProceso);
	t_pcb * procesoAEjecutar = obtenerSiguienteDeReady();
	pthread_mutex_unlock(&obtenerProceso);
	log_info(logger,
			"[readyAExe]: Desde readyAExe obtuvimos el proceso de pid: %d",
			procesoAEjecutar->idProceso);


	log_info(logger,
			"[readyAExe]: Desde readyAExe mandamos al proceso de pid %d a ejecutar  ",
			procesoAEjecutar->idProceso);
	ejecutar(procesoAEjecutar);

	log_info(logger,"[readyAExe]: finaliza");
	}
}

t_pcb* obtenerSiguienteFIFO() {

t_pcb* procesoPlanificado = list_remove(colaReady, 0);
log_info(logger, "[obtenerSiguienteFIFO]: PROCESOS EN READY: %d \n",
		list_size(colaReady));

return procesoPlanificado;
}

t_pcb* obtenerSiguienteSRT() {

t_pcb* procesoPlanificado = NULL;
t_pcb* procesoAux = NULL;
int i;
int indexARemover;
float shortestJob;

procesoAux = list_get(colaReady, 0);

indexARemover = 0;
shortestJob = procesoAux->estimacion_rafaga;

//itero por la lista de Ready

log_info(logger, "[obtenerSiguienteSRT]: PROCESOS EN READY: %d \n",
		list_size(colaReady));

for (i = 1; i < list_size(colaReady); i++) {
	procesoAux = list_get(colaReady, i);
	if (shortestJob > procesoAux->estimacion_rafaga) {
		shortestJob = procesoAux->estimacion_rafaga;
		indexARemover = i;
	}
}
procesoPlanificado = list_remove(colaReady, indexARemover);

return procesoPlanificado;
}

t_pcb* obtenerSiguienteDeReady() {

t_pcb * procesoPlanificado;

if (!strcmp(algoritmoPlanificacion, "FIFO")) {
	procesoPlanificado = obtenerSiguienteFIFO();
} else {
	if (!strcmp(algoritmoPlanificacion, "SRT")) {
		procesoPlanificado = obtenerSiguienteSRT();
	} else {
		log_info(logger, "[obtenerSiguienteDeReady]: algoritmo invalido");
		}
	}
	return procesoPlanificado;
}

void ejecutar(t_pcb* proceso) {

	if (proceso != NULL) {

		if (algoritmoPlanificacion == SRT) {
			log_info(logger,
					"[EXEC] Ingresa el proceso de PID: %d con una rafaga de ejecucion estimada de %f milisegundos.",
					proceso->idProceso,
					proceso->estimacion_rafaga);
		} else {
			log_info(logger, "[EXEC] Ingresa el proceso de PID: %d",
					proceso->idProceso);
		}

	} else {
		log_info(logger, "[EXEC] No se logró encontrar un proceso para ejecutar");
	}

	conexionConCpu(proceso);

	//log_info(logger, "[ejecutar]: enviamos el proceso a cpu");

	ejecucionActiva = true;
	log_info(logger, "[ejecutar]: ejecucion activa es true!!");

time_t a = time(NULL);
pthread_mutex_lock(&ejecucion);
proceso->horaDeIngresoAExe = ((float) a) * 1000;
proceso->estado = EXEC;
pthread_mutex_unlock(&ejecucion);

log_info(logger, "[ejecutar]: Se cambia el estado a EXEC y se calcula el inicio de ejecucion");

}

void atender_interrupcion_de_ejecucion() {

while (1) {
	int cod_op = recibir_operacion(socketCpuDispatch);

	switch (cod_op) {	// cuando reciba del cpu una interrupcion
	// obtener tiempo a partir de un recibir_tiempo()
	case I_O:
		log_info(logger,"[atender_interrupcion_de_ejecucion]: recibimos operacion IO");
		pthread_mutex_lock(&mutexIO);
		agregarABlocked(tomar_pcb(socketCpuDispatch));
	//	tiempoBloqueo = recibir_tiempo(); // recibir_paquete_int()?
		pthread_mutex_unlock(&mutexIO);
		log_info(logger,"[atender_interrupcion_de_ejecucion]: recibimos tiempo de bloqueo %d", tiempoBloqueo);
		sem_post(&pcbBlocked);
		log_info(logger,"[atender_interrupcion_de_ejecucion]: despertamos atenderIO");
		break;
	case EXIT:
		log_info(logger,"[atender_interrupcion_de_ejecucion]: recibimos operacion EXIT");
		terminarEjecucion(tomar_pcb(socketCpuDispatch));
		break;

	case INTERRUPT:

		log_info(logger,"[atender_interrupcion_de_ejecucion]: recibimos operacion INTERRUPT");
		pthread_mutex_lock(&mutexInterrupt);
		procesoDesalojado = tomar_pcb(socketCpuDispatch); //recibir_pcb
		pthread_mutex_unlock(&mutexInterrupt);
		log_info(logger,"[atender_interrupcion_de_ejecucion]: despertamos atenderDesalojo");
		sem_post(&pcbInterrupt);
		break;

	default:
		//log_info(logger, "operacion invalida");

		break;
	}
	ejecucionActiva = false;
	//log_info(logger,"[atender_interrupcion_de_ejecucion]: ejecucionActiva es false");
	sem_post(&cpuDisponible);
	//log_info(logger,"[atender_interrupcion_de_ejecucion]: a partir de ahora CPU DISPONIBLE!");

	}
}

void atenderIO() {
while (1) {
	sem_wait(&pcbBlocked);

	log_info(logger,"[atenderIO]: se desperto atenderIO");
	time_t a = time(NULL);
	float tiempoDeFin = ((float) a) * 1000;

	log_info(logger,"[atenderIO]: calculamos el tiempo de fin en %f milisegundos ", tiempoDeFin);
	pthread_mutex_lock(&bloqueandoProceso);
	t_pcb * procesoABloquear = queue_pop(colaBlocked);
	pthread_mutex_unlock(&bloqueandoProceso);

	log_info(logger,"[atenderIO]: sacamos al proceso %d de la cola de bloqueados  ", procesoABloquear->idProceso);
	procesoABloquear->rafagaMs = tiempoDeFin
			- procesoABloquear->horaDeIngresoAExe;

	estimarRafaga(procesoABloquear);

	log_info(logger,"[atenderIO]: estimamos la rafaga del proceso %d y da %f  ",procesoABloquear->idProceso, procesoABloquear->estimacion_rafaga );

	int tiempoDeBloqueo = min(tiempoMaximoBloqueado,
			obtenerTiempoDeBloqueo(procesoABloquear));

	log_info(logger,"[atenderIO]: Bloqueamos el proceso %d por un tiempo de %d  ", procesoABloquear->idProceso, tiempoDeBloqueo);

	usleep(tiempoDeBloqueo * 1000);

	log_info(logger,"[atenderIO]: Fin del bloqueo del proceso %d", procesoABloquear->idProceso);

	sacarDeBlocked();



	if (!supera_tiempo_maximo_bloqueado(procesoABloquear)) {

		log_info(logger,"[atenderIO]: El proceso %d no se suspende y se agrega a la cola de ready", procesoABloquear->idProceso);

		agregarAReady(procesoABloquear);

	} else {
		enviar_mensaje("Suspendo proceso, espero confirmación",socketMemoria,MENSAJE_LIBERAR_POR_SUSPENDIDO); //podria mandarse un pid
		// que lo reciba memoria
		int cod_op = recibir_operacion(socketMemoria);

		if(cod_op == MENSAJE_CONFIRMACION_SUSPENDIDO){
			recibir_mensaje(socketMemoria);
		}

		//Al suspenderse un proceso se enviará un mensaje a Memoria con la información necesaria y se esperará la confirmación del mismo.

		log_info(logger,"[atenderIO]: El proceso %d se suspende y se agrega a la cola de suspendedBlocked", procesoABloquear->idProceso);

		agregarASuspendedBlocked(procesoABloquear);

		log_info(logger,"[atenderIO]: despertamos a desbloquear_suspendido");
		sem_post(&suspAReady);

		}

	//	free(procesoABloquear);

	}
}

void desbloquear_suspendido() {
	while (1) {
		sem_wait(&suspAReady);

		log_info(logger,"[desbloquear_suspendido]: se desperto");
		t_pcb * proceso = queue_pop(colaSuspendedBlocked);

		log_info(logger,"[desbloquear_suspendido]: sacamos al proceso %d de la cola de suspendedBlocked", proceso->idProceso);

		log_info(logger,"[desbloquear_suspendido]: suspendemos al proceso %d, por un tiempo de %d", proceso->idProceso,(obtenerTiempoDeBloqueo(proceso) - tiempoMaximoBloqueado)
				* 1000);
		usleep(
					(obtenerTiempoDeBloqueo(proceso) - tiempoMaximoBloqueado)
							* 1000);

		log_info(logger,"[desbloquear_suspendido]: finaliza suspension");

		agregarASuspendedReady(proceso);

		log_info(logger,"[desbloquear_suspendido]: despertamos a asignar_memoria");

		sem_post(&pcbEnNew);
	}
}



void atenderDesalojo() {

while (1) {

	sem_wait(&desalojarProceso);

	log_info(logger, "[atenderDesalojo]: se desperto atenderDesalojo");

	t_pcb * proceso = list_get(colaReady, 0);

	log_info(logger,
			"[atenderDesalojo]: se despierta al simulador de cpu para continuar con el desalojo");
	//simulador_de_cpu(2, proceso);

	//socketCpuInterrupt = crear_conexion(ipCpu, puertoCpuInterrupt); // se crea la conexion con el puerto interrupt

	enviar_mensaje("Desalojar proceso",socketCpuInterrupt,MENSAJE_INTERRUPT); //chequear la interrupcion

	sem_wait(&pcbInterrupt);

	time_t a = time(NULL);
	float tiempoDeFin = ((float) a) * 1000;

	pthread_mutex_lock(&desalojandoProceso);
	procesoDesalojado->rafagaMs = tiempoDeFin
			- procesoDesalojado->horaDeIngresoAExe;
	estimarRafaga(procesoDesalojado);

	list_add(colaReady, procesoDesalojado);
	log_info(logger, "[READY] Entra el proceso de PID: %d a la cola.",
			procesoDesalojado->idProceso);

	pthread_mutex_unlock(&desalojandoProceso);
	log_info(logger,
			"[atenderDesalojo]: Calculamos la rafaga del proceso a desalojar: PID[%d] - ESTIMACION-RAFAGA[%f]",
			procesoDesalojado->idProceso, procesoDesalojado->estimacion_rafaga);
	sem_post(&procesoDesalojadoSem);
	log_info(logger, "[atenderDesalojo]: volvemos a despertar asignar_memoria");
}
}

void estimarRafaga(t_pcb* proceso) {

proceso->estimacion_rafaga = alfa * proceso->rafagaMs
		+ (1 - alfa) * proceso->estimacion_rafaga;

}

void terminarEjecucion(t_pcb * procesoAFinalizar) {

	log_info(logger, "[terminarEjecucion]: Se desperto terminarEjecucion...");
	t_pidXsocket * unaConsola;
	pthread_mutex_lock(&procesoExit);
	procesoAFinalizar->estado = TERMINATED;
	pthread_mutex_unlock(&procesoExit);
	log_info(logger,
			"[terminarEjecucion]: Se cambia el estado del proceso %d a TERMINATED",
			procesoAFinalizar->idProceso);

	enviar_mensaje("Liberar estructuras",socketMemoria,MENSAJE_LIBERAR_POR_TERMINADO);// que lo reciba memoria

	log_info(logger,
			"[terminarEjecucion]: Se envia un mensaje a memoria para que libere estructuras");

	bool consola_tiene_pid(t_pidXsocket * elemento) {
			return elemento->pid == procesoAFinalizar->idProceso;
		}

	pthread_mutex_lock(&consolasExit);
	unaConsola = list_remove_by_condition(listaDeConsolas, consola_tiene_pid); // segundo parametro poner void*
	pthread_mutex_unlock(&consolasExit);

	log_info(logger,
			"[terminarEjecucion]: Encontramos el socket del proceso %d, resulta que es %d",
			unaConsola->pid, unaConsola->socket);

	//	enviar_mensaje("Finalizo la ejecucion",unaConsola->socket,MENSAJE_FINALIZAR_EXE); //TODAVIA FALTA HACER QUE CONSOLA PUEDA RECIBIR MENSAJES
	//	liberar_conexion(unaConsola->socket);

	log_info(logger,
			"[terminarEjecucion]: Aumenta grado de multiprogramacion actual y termina terminarEjecucion!!");
	sem_post(&gradoDeMultiprogramacion);

	destruirProceso(procesoAFinalizar);
	free(unaConsola);

}


