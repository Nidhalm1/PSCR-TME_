# TME4 Answers

Tracer vos expériences et conclusions dans ce fichier.

Le contenu est indicatif, c'est simplement la copie rabotée d'une IA, utilisée pour tester une version de l'énoncé.
On a coupé ses réponses complètes (et souvent imprécises voire carrément fausses, deadlocks etc... en Oct 2025 les LLM ont encore beaucoup de mal sur ces questions, qui demandent du factuel et des mesures, et ont de fortes tendances à inventer).
Cependant on a laissé des indications en particulier des invocations de l'outil possibles, à adapter à votre code.

## Question 1: Baseline sequential

### Measurements (Release mode)

**Resize + pipe mode:**
```

-le wall clock c'est le temps passé entre le debut et la fin 
-cpu time c'est le temps passé par le cpu 
- le peak memoire c'est plus le progrmame (diffrent thread et tt) chargent les fichers , les traitent plus ca augment donc plusieur tread = big taux

./build/TME4 -m resize -i input_images -o output_images
... trace
wall clock: 6678 ms
cpu : 6395 ms CPU
Peak: 131 MB

./build/TME4 -m pipe -i input_images -o output_images
(wall clock): 6661 ms
(main): 4 ms CPU
Peak: 142 MB

```


## Question 2: Steps identification

I/O-bound: 
findImageFiles
load image
saveImage 

CPU-bound: 
findImageFiles
resizeImage


parallelisable a priori ?
oui puisque on a plusieurs image et on peut apple la fonction findImageFiles (qui charge, redimonsionne et sauvegarde pour chaque image dans l'input donné) avec plusieurs thread mais faudra passer une parti du dossier ,coiper le dossier sur plusieurs input



## Question 3: BoundedBlockingQueue analysis
oui il supporte plusieurs producteur et consommateurs puisque ya une section crtitque et c'est protegé .
un appel peut bloquer dans lecas ou la file est vide ou pleine
alors la lambda , capture tout de this, teste un cas si c'est vrai elle continue avec le verou sinon elle lache le verrou jusqu'a elle recoit un notify all qui qui va la reveiller et et elle reteste lacondition puis push(ou pull)


## Question 4: Pipe mode study

FILE_POISON ...

Order/invert :
file poison elle sert à dire que le producteur a fini de push pour dire que le thread n'attend plus pour pop , c'est un simpleemnt un shema vide donc definie dans tastk
le main push plein e fichier jusqu'a la fin et push poison pour dire que j'ai terminé comme ca le thread consomateur n'attend plus puisque il fait cv.wait(l) puis recoit le posion et teste si file = poisson donc s'arrte
2 et 3  oui mais ca sert à rien donc ca va chercher tt le fichier puis traiter 
3 et 4  non car ca va arreter le tratmenet et 4 et 5 non


## Question 5: Multi-thread pipe_mt

Implement pipe_mt mode with multiple worker threads.

For termination, ... poison pills...
j'ai juste ajouté n POISON pour dire aux n thead qu'ils ont terminé
Measurements:
- N=1: 
```
./build/TME4 -m pipe_mt -n 1 -i input_images -o output_images
...7000
```
- N=2: 
```
./build/TME4 -m pipe_mt -n 2 -i input_images -o output_images
...3600
```
- N=4: 
```
./build/TME4 -m pipe_mt -n 4 -i input_images -o output_images
...1900
```
- N=8: 
```
./build/TME4 -m pipe_mt -n 8 -i input_images -o output_images
...1600
```

Best: ??    
meilleur n = 8 car plus n augmente les thread plus le peak augmente et donc pour charger les donné les I/O prennet plus du temps 
Les threads se battent pour le disque (I/O) puisque peuvent sauvegarder au mm temps → lecture/écriture ralentie,
Ils utilisent plus de mémoire (plus d’images en même temps),

-cette methodeest pas si bien car pendant qu'un thread load si le disquemet du temps donc le deuxiemen va attendre  
- la prochaine est mieux car y aura plus c blocage puisque plieux thread chargnet lesimages etlesautes iront voir dans ce qu y est chargé 
- donc pendant ce temps au lieux qu'il soit bloqué il peut redimensionner les images 

## Question 6: TaskData struct

-- en fait au debut ils vont tous charger les images donc le disque saturee mets plus de temps puis traiter tous les images cpu saturee ect... alors 
que la methode pipline certains vont arriver charger les images et pendant que ca charge certians decharge la file pour traiter et pendant ce traitement certaint enregistre donc le cpu et I O travaille equitablement genre pas saturéé donc on gagne

```cpp
struct TaskData {
    QImage image;
    std::filesystem::path file;
    TaskData() = default ;
    TaskData(const QImage& image, const std::filesystem::path& file)
        : image(image), file(file) {}
};
```
PAS DE POINTEUR CAR Qimage ne cree pas une cope à chaque push simplement si ya une modififcation et dans notre cas nn donc on perd pas de la memoire et du temps  en copiant à chaque fois ( 10 mo par exemple )
Fields: QImage ??? for the image data, ...
Q image pour chaque etape puisque ya des image redimonsioné et pour d'autre queue nn
Use ??? for QImage, because ... copier une image a chaque push c'est loud mais q image ne le fait pas elle cree une "fausse" copie dans on perd pas du temps à recopier 

TASK_POISON: ...def...

## Question 7: ImageTaskQueue typing

pointers vs values ; alors  using ImageTaskQueue = BoundedBlockingQueue<TaskData> est plus simple mais copie l'objet à chaque push c'est ce qui peut etre lourd sauf que QIMAGE est copy-on-write donc ne copie pas vraiment  et en plus code plus simple et lisible sans se sousier des fuites memoires alors que using ImageTaskQueue = BoundedBlockingQueue<TaskData*>; certain ne cree pas de copie ect mais risque de fuite memoire et  il faut une gestion de cette dernierre 

Choose BoundedBlockingQueue<TaskData> as consequence

## Question 8: Pipeline functions

Implement reader, resizer, saver in Tasks.cpp.

mt_pipeline mode: Creates threads for each stage, with configurable numbers.

Termination: Main pushes the appropriate number of poisons after joining the previous stage.

Measurements: 
```
./build/TME4 -m mt_pipeline -i input_images -o output_images
...
```


## Question 9: Configurable parallelism

Added nbread, nbresize, nbwrite options.


Timings:
- 1/1/1 (default): 
```
./build/TME4 -m mt_pipeline -i input_images -o output_images
...
```
- 1/4/1: 
```
./build/TME4 -m mt_pipeline --nbread 1 --nbresize 4 --nbwrite 1 -i input_images -o output_images
```

- 4/1/1: 
```
./build/TME4 -m mt_pipeline --nbread 4 --nbresize 1 --nbwrite 1 -i input_images -o output_images
```
... autres configs

Best config: 
interprétation
La configuration 2/1/1 est la plus rapide.
Cela montre que l’étape reader (chargement et décodage des images) est le goulot d’étranglement du pipeline.
C’est elle qui limite le débit global, car les accès disque et le décodage JPEG sont beaucoup plus lents que le redimensionnement ou l’écriture.

## Question 10: Queue sizes impact


With size 1: 
```
./build/TME4 -m pipe_mt -n 2 --queue-size 1 -i input_images -o output_images
...
```

With size 100: 
```
./build/TME4 -m pipe_mt -n 2 --queue-size 100 -i input_images -o output_images
...
```

impact

Complexity: 


## Question 11: BoundedBlockingQueueBytes

Implemented with byte limit.

mesures

## Question 12: Why important

Always allow push if current_bytes == 0, ...

Fairness: ...

## Bonus

