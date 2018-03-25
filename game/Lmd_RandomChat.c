
#ifdef LMD_EXPERIMENTAL

#include "g_local.h"
#include "Lmd_Arrays.h"



/*

rchat/sentances.txt:
#person# is #adjective#ing #position# #location#.
#pronoun# #profession title# says that #food# is #description#

rchat/words/person.txt
Phred
Lugor
FlipSide
#CHATTER# //the name of the one who is chatting
#OTHER# //randomly select a name (not your own) from the players on the server.

rchat/words/adjective.txt:
dance
eat
sleep
exit
explode

rchat/words/position.txt:
above
below
to the left of
around #RND?100:300#.#RND?0:9##RND?0:9##RND?3:9# miles north of //#RND?100:300# = random number between 100 and 300 inclusive.

rchat/words/location.txt:
Epcot
space
France
#SERVER# //name of the server

rchat/words/pronoun.txt:
my
your
his
her

rchat/words/profession_title.txt
anesthesiologist
anthropologist
biologist
cosmetologist
geologist
metrologist
necrologist
meteorologist

rchat/words/food.txt
egg
cheese
toast

rchat/words/description.txt
overglorified
easy on the eyes
too small
too hard
segmentated
intoxicating

special modifiers inside the words files
#SERVER# //name of the server
#CHATTER# //the name of the one who is chatting
#OTHER# //randomly select a name (not your own) from the players on the server.
#RND?min:max# //random number between min and max inclusive.
*/

typedef struct wordArray_s{
	char *name;
	unsigned int count;
	char **words;
}wordArray_t;

struct wordGroups_s{
	unsigned int count;
	wordArray_t *Arrays;
}wordGroups;

#endif