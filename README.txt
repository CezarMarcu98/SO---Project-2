Marcu-Nicolescu Cezar-George
335CB

Structura definita de mine este formata din :
int fd; -> file descriptorul file-ului
unsigned char buffer[4096]; -> bufferul in care retin caracterele itite / scrise
size_t size; -> dimensiunea actuala a buffer-ului (in functie de cate elemente citeste din fisier , bufferul poate sa aiba pana la maxim 4096 de caractere
int index; -> variabila cu care retin pozitia in buffer
unsigned int w; -> variabila care imi indica in ce format a fost deschis fisierul ( 1 pt write, 2 pt append, 0 pt read ), ma ajuta pentru a face scrierea pe fisiere ce au fost deschise pentru scriere
int fseekPos; -> indicator de ftell . Pozitia in fisierul din care citesc, indiferent daca s-au produs scrieri/ citiri
int lastOp; -> ultima operatie efectuata asupra file-ului din care citesc, ma ajuta pt a invalida buffer-ul la fseek
bool fEND; -> variabila care imi spune daca am ajuns la sfarsit de fisier . Verific in functie de return-ul read-ului. Daca mi-a intors 0 read , inseamna ca nu mai are ce caractere citi
bool fERR; -> aceasta este setata pe 1 daca s-au produs erori ulterioare in cod . Mai ales la fflush , scrieri sau citiri.
pid_t child_pid; -> retine pid-ul procesul copilului pentru a-i astepta terminarea executiei dupa inchiderea stream-ului curent.

In fopen mi-am alocat memoria pt structura mea de tip so_file. Am deschis fisierul pe care lucrez in functie de parametrii primiti de functia open si mi-am atribuit cateva valori necesare ulterioarelor prelucrari cu fisierle. Cum ar fi file->w ce imi spune in ce format a fost deschis fisierul . Daca accepta scrieri sau nu.

In fgetc fac citiri fin file in buffer in situatia in care index-ul meu a ajuns la pozitia maxima a buffer-ului sau in cazul in care bufferul a fost invalidat / este gol. Apoi returnez caracterul de pozitia indexului curent.

Fputc-ul imi verifica daca bufferul este plin . In caz afirmativ imi scrie inapoi in fisier prin fflush , iar apoi adauga elementul primit ca parametru in bufferul meu.

So_fread si so_fwrite utilizeaza un for in care se bucleaza apelul functiilor fgetc , respectiv fputc de nmemb * size (pt ca buffer-ul meu este format din unsigned char-uri) si imi verifica daca pe parcursul executiei for-ului au aparut erori (cu stream->fERR la scriere sau citire) cat si daca am ajuns la EOF, caz in care imi returneaza pozitia pana la care s-a facut scrierea/ citirea.

In fflush verific ultima operatie executata . Daca este scriere invalidez buffer-ul cu memset, daca este citire incep scrierea in fisier. Verific numarul de elemente scrise pentru a-mi da seama daca a aparut cumva o eroare pe care ar trebui s-o returnez.

Fclose-ul imi inchide fisierele dupa ce a scris tot in fisier, cu fflush . Imi dezaloca spatiul de memorie pentru stream-ul curent si verifica daca return-urile close-ului sunt valide (fisierul a fost inchis cu succes).

Fseek : functie in care folosesc lseek-ul dupa ce actualizez de fiecare data bufferul cu so_fflush . In aceasta functie imi actualizez variabila pentru ftell in cazul in care a fost apelat un fseek ( se muta pointerul din fisier ).

La so_popen am preferat sa pun si cateva comentarii , consider ca a fost cea mai grea functie de implementat si am vrut sa fie explicata si pe cod . Am creat un pipe , iar apoi , am folosit functia fork pentru a crea un copil al procesului actual . In switch , pe cazul in care apelul fork esueaza am inchis caile de acces la pipe . Situatia in care apelul fork a reusit am tratat-o astfel : mai intai am verificat type-ul fisierului din parametru, si, in functie de acesta , am redirectat intrarile pipe-ului la stdin / stdout , ca apoi sa apelez comanda.
Cazul default , cel al parintelui , imi creeaza structura so file (alocare de memorie si atribuiri de variabile) si imi retine pid-ul copilului in file-ul folosit. La fel , in functie de tip, am legat file descriptorul fisierului la capatul pipe-ului folosit.


Functia so_pclose imi sterge structura alocata in popen si asteapta terminarea procesului copil , imi returneaza statusul copilui sau , in cazul in care apelul wait a esuat eroare.
