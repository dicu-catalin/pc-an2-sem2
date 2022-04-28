	Aplicatia client-server indeplineste aproape toate cerintele temei,
mai putin partea de quick_flow.
	
	Serverul deschide doi socketi, unul pentru UDP si unul pentru TCP.
Pe socketul TCP primeste conexiuni TCP noi. In cazul in care o conexiune este
acceptata, adauga clientul in lista de clienti. Daca un client se aboneaza la
un topic care nu exista, topicul se creeaza si este adaugat in lista de 
topicuri. Daca se primesc date pe socketul UDP, serverul transmite mesajul
mai departe tuturor clientilor activi care sunt abonati la topic. Daca un 
client este abonat la topic, dar este inactiv, mesajele sunt stocate in
memorie pentru a fi trimise ulterior. Daca serverul se inchide, se inchid toti
clientii inca deschisi.

	Clientul deschide un socket si realizeaza comexiunea cu serverul.
Acesta ii trimite un pachet pe noul socket deschis care contine id-ul sau.
Daca in client se primesc date de la tastatura, acestea pot fi pentru 
subscribe/unsubscribe sau exit si sunt tratate de client. Daca se primesc
date pe socketul deschis, poate sa fie o cerere de la server pentru a inchide
clientul sau poate fi un mesaj de pe un topic la care clientul este abonat.
