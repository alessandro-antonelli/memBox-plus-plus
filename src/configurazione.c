/*
 * membox++ Progetto del corso di LSO 2016 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Pelagatti, Torquati
 *
 *
 * Studente: Alessandro Antonelli
 * Matricola 507264
 *
 * Si dichiara che il programma è, in ogni sua parte,
 * opera originale dell'autore.
 * -- Alessandro Antonelli
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "configurazione.h"

/*
     LeggiConfigurazione
Legge il file di configurazione all'indirizzo indicato in path e legge da esso le impostazioni, salvandole nella struttura puntata da config
	Valori di ritorno
	0	tutte le 8 impostazioni lette e parsate correttamente
	2	file malformato: una o più impostazioni fondamentali mancanti, il server non potrà avviarsi
	3	file parsato ma alcune delle impostazioni "opzionali" (StatFileName, DumpFile) mancavano dal file
*/
int LeggiConfigurazione(char* path, config_t* config)
{
	int IsSettingMatched[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; //i-esimo elem dell'array = 0 significa che la i-esima impostazione non è stata ancora trovata nel file di configurazione

	//apro il file
	FILE* configFile = fopen(path, "r");
	CheckExit(configFile == NULL, TRUE, "apertura file di configurazione");

	//leggo linea
	char line[MAXLINELEN];
	if(fgets(line, MAXLINELEN, configFile) == NULL) { fclose(configFile); return 2; }

	do
	{
		//INTERPRETO LINEA
		char nomePar[MAXLINELEN] = "";
		char valPar[MAXLINELEN] = "";
		int i=0, j=0, k=0; //indici di: lettura dalla linea, scrittura nome parametro, scrittura valore parametro
		int uguale = FALSE; //indica se nella lettura della riga ho incontrato '='
		char caratt = line[0]; //carattere della linea attualmente esaminato
		for(i=0; caratt != '\0' && caratt != '\n' && caratt != EOF; i++)
		{
			//INTERPRETO SINGOLO CARATTERE DELLA LINEA
			caratt = line[i];

			if(caratt == '#') break; //salto tutta la linea, commento
			if(caratt == ' ' || caratt == 9) { continue; } //salto spazi e TAB
			if(caratt == '=') { uguale=TRUE; continue; }
			if(caratt == '\n' || caratt == '\0' || caratt == EOF) { break; } //ho raggiunto la fine della linea

			if(!uguale) { nomePar[j] = caratt; j++; }
			else { valPar[k] = caratt; k++; }
		}

		if(j > 0) //se la linea era un commento si passa subito alla successiva
		{
			nomePar[j] = '\0';
			valPar[k] = '\0';

			//RICONOSCO CONTENUTO
			if(strcmp(nomePar, "UnixPath") == 0)
			{
				config->UnixPath = malloc(strlen(valPar) + sizeof(char)); //+1 char per il terminatore \0
				CheckExit(config->UnixPath == NULL, FALSE, "malloc config->UnixPath");
				strncpy(config->UnixPath, valPar, strlen(valPar) + sizeof(char));
				IsSettingMatched[0] = TRUE;
				continue;
			}
			if(strcmp(nomePar, "MaxConnections") == 0) { config->MaxConnections = atoi(valPar); IsSettingMatched[1] = TRUE; continue; }
			if(strcmp(nomePar, "ThreadsInPool") == 0) { config->ThreadsInPool = atoi(valPar); IsSettingMatched[2] = TRUE; continue; }
			if(strcmp(nomePar, "StorageSize") == 0) { config->StorageSize = atoi(valPar); IsSettingMatched[3] = TRUE; continue; }
			if(strcmp(nomePar, "StorageByteSize") == 0) { config->StorageByteSize = atoi(valPar); IsSettingMatched[4] = TRUE; continue; }
			if(strcmp(nomePar, "MaxObjSize") == 0) { config->MaxObjSize = atoi(valPar); IsSettingMatched[5] = TRUE; continue; }
			if(strcmp(nomePar, "StatFileName") == 0)
			{
				config->StatFileName = malloc(strlen(valPar) + sizeof(char)); //+1 char per il terminatore \0
				CheckExit(config->StatFileName == NULL, FALSE, "malloc config->StatFileName");
				strncpy(config->StatFileName, valPar, strlen(valPar) + sizeof(char));
				IsSettingMatched[6] = TRUE;
				continue;
			}
			if(strcmp(nomePar, "DumpFile") == 0)
			{
				config->DumpFile = malloc(strlen(valPar) + sizeof(char)); //+1 char per il terminatore \0
				CheckExit(config->DumpFile == NULL, FALSE, "malloc config->DumpFile");
				strncpy(config->DumpFile, valPar, strlen(valPar) + sizeof(char));
				IsSettingMatched[7] = TRUE;
				continue;
			}
		}

	} while(fgets(line, MAXLINELEN, configFile) != NULL);

	CheckExit(fclose(configFile) == EOF, TRUE, "chiusura file di configurazione");

	//Restituisco errore 2 in caso di valori non validi
	if(IsSettingMatched[1] && config->MaxConnections <= 0) return 2;
	if(IsSettingMatched[2] && config->ThreadsInPool <= 0) return 2;
	if(IsSettingMatched[3] && config->StorageSize < 0) return 2;
	if(IsSettingMatched[4] && config->StorageByteSize < 0) return 2;
	if(IsSettingMatched[5] && config->MaxObjSize < 0) return 2;

	//Restituisco 0 (successo) 2 (malformato) o 3 (incompleto) in base alla presenza/assenza delle impostazioni dal file letto
	if(IsSettingMatched[0] && IsSettingMatched[1] && IsSettingMatched[2] && IsSettingMatched[3] && IsSettingMatched[4] && IsSettingMatched[5])
	{
		if(IsSettingMatched[6] && IsSettingMatched[7]) return 0; //tutte le impostazioni lette
		else return 3; //quelle fondamentali ci sono, ne manca almeno una opzionale
	}
	else return 2; //file malformato, manca almeno un'impostazione fondamentale
}
