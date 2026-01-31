# Gestione Spese Personali

Il progetto consiste in un’applicazione sviluppata in linguaggio C++ per la gestione delle spese personali tramite interfaccia testuale (console) e database relazionale SQLite.

L’obiettivo è consentire a un singolo utente di registrare le proprie spese, organizzarle per categoria, definire un budget mensile e analizzare il proprio comportamento di spesa attraverso report riepilogativi.

---

## Descrizione del funzionamento

Il programma viene eseguito da console e guida l’utente tramite un menu testuale.
All’avvio viene mostrato un messaggio di benvenuto e viene richiesta una scelta iniziale per il caricamento dei dati di esempio.

Le categorie di spesa utilizzate nel progetto sono:
- Alimenti
- Sport
- Affitto
- Extra

Per ciascuna categoria è possibile inserire spese giornaliere e definire un budget mensile.

---
## Compilazione del programma e avvio

Aprire il terminale, posizionarsi nella cartella contenente il file sorgente ed eseguire il comando:

```bash
g++ main.cpp -lsqlite3 -o spese
./spese 

Prima dell’esecuzione del programma, viene visualizzato il seguente messaggio:

Vuoi caricare i dati di esempio? (S/N)

- Inserendo S (Sì), il programma carica automaticamente nel database alcune categorie, spese e budget di esempio, utili per testare immediatamente le funzionalità.
- Inserendo N (No), il programma viene avviato con un database vuoto e l’utente può inserire manualmente tutti i dati.

Dopo questa scelta viene mostrato il menu principale.

---

## Funzionalità principali

Il menu principale consente di accedere alle seguenti funzionalità:

- Gestione delle categorie di spesa, con controllo dell’esistenza per evitare duplicati
- Inserimento di una nuova spesa specificando:
  - data
  - importo
  - categoria
  - descrizione facoltativa
- Definizione di un budget mensile per ciascuna categoria
- Visualizzazione dei report:
  - totale delle spese per categoria
  - confronto tra spese mensili e budget
  - elenco completo delle spese ordinate per data

Tutte le scelte dell’utente vengono acquisite tramite input da tastiera e gestite mediante strutture di controllo.






