#! /bin/bash

# Progetto Membox++ - insegnamento di Sistemi Operativi - anno accademico 2015/2016
# Autore: Alessandro Antonelli, matricola 507264
# Si dichiara che questo script è interamente opera originale dell'autore

SCRIPTNAME=${0##*/}

#Nessun argomento, oppure unico argomento --help
if [ $# = 0 ] || ( [ $# = 1 ] && [ $1 = '--help' ] ); then
	echo -e "\t========================\n\t=== Uso dello script ===\n\t========================" 1>&2

	echo -e "(1)\t$SCRIPTNAME statfile\nStampa le statistiche relative all'ultimo timestamp (tutte)\n" 1>&2

	echo -e "(2)\t$SCRIPTNAME statfile [ -p -u -g -r -c -s -o -m -d ]\nStampa le statistiche relative a tutti i timestamp (solo quelle corrispondenti alle opzioni scelte)\n" 1>&2

	echo -e "(3)\t$SCRIPTNAME --help\nFa comparire questo messaggio" 1>&2

	exit 0
fi

#Le seguenti variabili memorizzano gli argomenti passati allo script
STATFILE=''
PUT=NO
UPDATE=NO
GET=NO
REMOVE=NO
CONNESSIONI=NO
SIZE=NO
NUMOGGETTI=NO
MAX=NO
DUMP=NO

for argomento do
	if [ $argomento = '-p' ]; then
		PUT=SI
	elif [ $argomento = '-u' ]; then
		UPDATE=SI
	elif [ $argomento = '-g' ]; then
		GET=SI
	elif [ $argomento = '-r' ]; then
		REMOVE=SI
	elif [ $argomento = '-c' ]; then
		CONNESSIONI=SI
	elif [ $argomento = '-s' ]; then
		SIZE=SI
	elif [ $argomento = '-o' ]; then
		NUMOGGETTI=SI
	elif [ $argomento = '-m' ]; then
		MAX=SI
	elif [ $argomento = '-d' ]; then
		DUMP=SI
	elif [ -f $argomento ] && [ -r $argomento ]; then
		STATFILE=$argomento
	else
		echo "${SCRIPTNAME}: Errore, il file specificato (\"$argomento\") non esiste o non si dispone dei diritti di lettura" 1>&2
		exit 1
	fi
done

#Se tra i parametri letti manca lo statfile esco e stampo errore
if [ -z $STATFILE ]; then
	echo "${SCRIPTNAME}: Errore, il parametro statfile è obbligatorio" 1>&2
	exit 1
fi

#Se il file è vuoto esco e stampo errore
if [ ! -s $STATFILE ]; then
	echo "${SCRIPTNAME}: Errore, il file specificato (\"$STATFILE\") è vuoto" 1>&2
	exit 1
fi

#Apro in lettura lo statfile
exec 3< $STATFILE

if [ $PUT = NO ] && [ $UPDATE = NO ] && [ $GET = NO ] && [ $REMOVE = NO ] && [ $CONNESSIONI = NO ] && [ $NUMOGGETTI = NO ] && [ $MAX = NO ] && [ $DUMP = NO ]; then
# E' stato specificato solo lo statfile, senza nessuna opzione
#	=============
# 	MODALITA' (1)
#	=============
# Stampo tutte le statistiche dell'ultimo timestamp

	while read -u 3 timestamp trattino put putf update updatef get getf remove removef lock lockf lockobj lockobjf dump dumpf conness size maxsize oggetti maxoggetti; do
		finaltimestamp=$timestamp
		finalput=$put
		finalputf=$putf
		finalupdate=$update
		finalupdatef=$updatef
		finalget=$get
		finalgetf=$getf
		finalremove=$remove
		finalremovef=$removef
		finallock=$lock
		finallockf=$lockf
		finallockobj=$lockobj
		finallockobjf=$lockobjf
		finaldump=$dump
		finaldumpf=$dumpf
		finalconness=$conness
		finalsize=$size
		finalmaxsize=$maxsize
		finaloggetti=$oggetti
		finalmaxoggetti=$maxoggetti
	done

	if [ -z $finaltimestamp ] || [ -z $finalput ] || [ -z $finalputf ] || [ -z $finalupdate ] || [ -z $finalupdatef ] || [ -z $finalget ] || [ -z $finalgetf ] || [ -z $finalremove ] || [ -z $finalremovef ] || [ -z $finallock ] || [ -z $finallockf ] || [ -z $finallockobj ] || [ -z $finallockobjf ] || [ -z $finaldump ] || [ -z $finaldumpf ] || [ -z $finalconness ] || [ -z $finalsize ] || [ -z $finalmaxsize ] || [ -z $finaloggetti ] || [ -z $finalmaxoggetti ]; then
		echo "${SCRIPTNAME}: Errore, il file specificato (\"$STATFILE\") non rispetta la formattazione attesa (è corrotto o non generato da membox++)" 1>&2
		exit 1
	fi

	#Converto in KB
	finalsizeKB=$(bc <<< "scale=2; $finalsize / 1024")
	finalmaxsizeKB=$(bc <<< "scale=2; $finalmaxsize / 1024")

	#Se il numero occupa più di 7 caratteri indico di inserire un nuovo carattere TAB
	if ((finalsize >= 10240000)); then
		eventualetab1='\t'
	else
		eventualetab1=''
	fi
	if ((finalmaxsize >= 10240000)); then
		eventualetab2='\t'
	else
		eventualetab2=''
	fi

	#stampo a schermo le informazioni
	echo -e "\t======================================================"
	echo -e "\t=== Statistiche dell'ultimo timestamp ($finaltimestamp) ==="
	echo -e "\t======================================================"

	echo -e "PUT\tPUTF\tUPDATE\tUPDATEF\tGET\tGETF\tREMOVE\tREMOVEF\tLOCK\tLOCKF"
	echo -e "$finalput\t$finalputf\t$finalupdate\t$finalupdatef\t$finalget\t$finalgetf\t$finalremove\t$finalremovef\t$finallock\t$finallockf"
	echo '' #riga vuota

	echo -e "LOCKobj\tLOCKobF\tDUMP\tDUMPF\tCONNESS\tSIZE_KB$eventualetab1\tMAXSIZE$eventualetab2\tOGGETTI\tMAX_OGG"
	echo -e "$finallockobj\t$finallockobjf\t$finaldump\t$finaldumpf\t$finalconness\t${finalsizeKB}\t${finalmaxsizeKB}\t$finaloggetti\t$finalmaxoggetti"

	exit 0
fi

#	=============
# 	MODALITA' (2)
#	=============
# Stampo le statistiche selezionate per tutti i timestamp

#Se c'è almeno una delle colonne, stampo intestazione
if [ $PUT = SI ] || [ $UPDATE = SI ] || [ $GET = SI ] || [ $REMOVE = SI ] || [ $CONNESSIONI = SI ] || [ $SIZE = SI ] || [ $NUMOGGETTI = SI ] || [ $DUMP = SI ]; then
	echo -n -e "\nTIMESTAMP"
	if [ $PUT = SI ]; then
		echo -n -e "\tPUT\tPUTF"
	fi
	if [ $UPDATE = SI ]; then
		echo -n -e "\tUPDATE\tUPDATEF"
	fi
	if [ $GET = SI ]; then
		echo -n -e "\tGET\tGETF"
	fi
	if [ $REMOVE = SI ]; then
		echo -n -e "\tREMOVE\tREMOVEF"
	fi
	if [ $CONNESSIONI = SI ]; then
		echo -n -e "\tCONNESS"
	fi
	if [ $SIZE = SI ]; then
		echo -n -e "\tSIZE_KB"
	fi
	if [ $NUMOGGETTI = SI ]; then
		echo -n -e "\tOGGETTI"
	fi
	if [ $DUMP = SI ]; then
		echo -n -e "\tDUMP\tDUMPF"
	fi
	echo '' #vado a capo
fi

MAX_connessioni=0
MAX_oggetti=0
MAX_sizeKB=0

while read -u 3 timestamp trattino put putf update updatef get getf remove removef lock lockf lockobj lockobjf dump dumpf conness size maxsize oggetti maxoggetti; do

	if [ -z $timestamp ] || [ -z $put ] || [ -z $putf ] || [ -z $update ] || [ -z $updatef ] || [ -z $get ] || [ -z $getf ] || [ -z $remove ] || [ -z $removef ] || [ -z $lock ] || [ -z $lockf ] || [ -z $lockobj ] || [ -z $lockobjf ] || [ -z $dump ] || [ -z $dumpf ] || [ -z $conness ] || [ -z $size ] || [ -z $maxsize ] || [ -z $oggetti ] || [ -z $maxoggetti ]; then
		echo "${SCRIPTNAME}: Errore, il file specificato (\"$STATFILE\") non rispetta la formattazione attesa (è corrotto o non generato da membox++)" 1>&2
		exit 1
	fi

	sizeKB=$(bc <<< "scale=2; $size / 1024")
	maxsizeKB=$(bc <<< "scale=2; $maxsize / 1024")

	if [ $PUT = SI ] || [ $UPDATE = SI ] || [ $GET = SI ] || [ $REMOVE = SI ] || [ $CONNESSIONI = SI ] || [ $SIZE = SI ] || [ $NUMOGGETTI = SI ] || [ $DUMP = SI ]; then
		echo -n $timestamp
		if [ $PUT = SI ]; then
			echo -n -e "\t${put}\t${putf}"
		fi
		if [ $UPDATE = SI ]; then
			echo -n -e "\t${update}\t${updatef}"
		fi
		if [ $GET = SI ]; then
			echo -n -e "\t${get}\t${getf}"
		fi
		if [ $REMOVE = SI ]; then
			echo -n -e "\t${remove}\t${removef}"
		fi
		if [ $CONNESSIONI = SI ]; then
			echo -n -e "\t${conness}"
		fi
		if [ $SIZE = SI ]; then
			echo -n -e "\t${sizeKB}"
		fi
		if [ $NUMOGGETTI = SI ]; then
			echo -n -e "\t${oggetti}"
		fi
		if [ $DUMP = SI ]; then
			echo -n -e "\t${dump}\t${dumpf}"
		fi
		echo '' #vado a capo
	fi

	#registro i picchi per l'opzione -m
	if (($conness > $MAX_connessioni)); then
		MAX_connessioni=$conness
	fi
	if test $(bc <<< "$maxoggetti > $MAX_oggetti"); then
		MAX_oggetti=$maxoggetti
	fi
	if test $(bc <<< "$maxsizeKB > $MAX_sizeKB"); then
		MAX_sizeKB=$maxsizeKB
	fi
done

if [ $MAX = SI ]; then
	#Unico comando che non va in colonna
	echo -e "\nN. max di connessioni:\t\t$MAX_connessioni"
	echo -e "N. max di oggetti memorizzati:\t$MAX_oggetti"
	echo -e "Massima size raggiunta:\t\t$MAX_sizeKB KB"
fi
