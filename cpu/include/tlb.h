#include "funcionesCpu.h"
//Inicializar para poder utilizar la cahce
t_list* inicializarTLB(){

	tlb = list_create();
	return tlb;
}

//Reiniciar la TLB cuando se hace process switch
void reiniciarTLB(){
	list_clean(tlb);
}

//Busqueda de pagina en la TLB
int chequearMarcoEnTLB(int numeroDePagina){
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));

	for(int i=0;i < list_size(tlb);i++){
		unaEntradaTLB = list_get(tlb,i);

			if(unaEntradaTLB->nroDePagina == numeroDePagina){
				unaEntradaTLB->ultimaReferencia = time(NULL);// se referencio a esa pagina
				return unaEntradaTLB->nroDeMarco; // se puede almacenar este valor en una variable y retornarlo fuera del for
			}
	}
	free(unaEntradaTLB);
	return -1;


}

void agregarEntradaATLB(int marco,int pagina){

	entradaTLB* unaEntrada = malloc(sizeof(entradaTLB));

	unaEntrada->nroDeMarco = marco;
	unaEntrada->nroDePagina = pagina;
	unaEntrada->instanteGuardada = time(NULL);
	unaEntrada->ultimaReferencia = time(NULL);

	list_add(tlb,unaEntrada);

}

void algoritmosDeReemplazoTLB(int pagina,int marco){//probarrrr-----------------------------------
	if(! strcmp(algoritmoReemplazoTlb,"FIFO")){
		entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));
		entradaTLB* otraEntradaTLB = malloc(sizeof(entradaTLB));

		int indiceConInstanteGuardadoMayor = 1;

		for(int i = 0; i<cantidadEntradasTlb;i++){
			unaEntradaTLB = list_get(tlb,i);
			otraEntradaTLB = list_get(tlb,indiceConInstanteGuardadoMayor);

			if(unaEntradaTLB->instanteGuardada < otraEntradaTLB->instanteGuardada){
				indiceConInstanteGuardadoMayor = i;
			}
		}

		reemplazarPagina(pagina,marco,indiceConInstanteGuardadoMayor);

	}else if (! strcmp(algoritmoReemplazoTlb,"LRU")){
		entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB)); //repito logica por la ultimaReferencia
		entradaTLB* otraEntradaTLB = malloc(sizeof(entradaTLB));

		int indiceConUltimaReferenciaMayor = 0;

		for(int i = 0; i<cantidadEntradasTlb;i++){
			unaEntradaTLB = list_get(tlb,i);
			otraEntradaTLB = list_get(tlb,indiceConUltimaReferenciaMayor);

			if(unaEntradaTLB->ultimaReferencia < otraEntradaTLB->ultimaReferencia){
				indiceConUltimaReferenciaMayor = i;
			}
		}

		reemplazarPagina(pagina,marco,indiceConUltimaReferenciaMayor);
	}
}

void reemplazarPagina(int pagina,int marco ,int indice){//Repito logica con agregarEntradaTLB
	entradaTLB* unaEntradaTLB = malloc(sizeof(entradaTLB));
	unaEntradaTLB = list_get(tlb,indice);

	log_info(logger,"TLB: Reemplazo la pagina %d en el marco %d cuya entrada es %d",pagina,marco,indice);

	unaEntradaTLB->nroDePagina = pagina;
	unaEntradaTLB->nroDeMarco = marco;
	unaEntradaTLB->instanteGuardada = time(NULL);
	unaEntradaTLB->ultimaReferencia = time(NULL);

	list_replace(tlb,indice,unaEntradaTLB);

}