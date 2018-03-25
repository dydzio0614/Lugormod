

/*

//REL: see Lmd_Invitations.h
Euka style faction system

This could be made more generic by merging property files directly into this (and calling it "group").
For a generic group:
Allow credits to be sent to players in the group when offline, for payable/rentable terminals with owners.
Allow credits to be divided amoungst players based on rank (20 precent for all players in rank x, 30 precent for rank y, 50 precent to general coffers).

Store players in the faction file, makes more sense in this case.


name: <name>
created: <created date>

//auto: join when meet criteria
//free: user selects it.
//locked: only people within the faction can add members.
//NOTE: dont allow members to add someone against their will, must be invite.
join: <auto, free, locked>
require_profession: <profession bitmask>
require_level: <level>

//Rank key with many to one ratio
//flags:
//1: dont show in rank list for users without rank edit premissions
//2: auto get this level when conditions are met (warning: will deduct credits without asking if credits req is set).
//Hiarchy level controls who can mess with who in the faction.  Like authfiles, ranks are not level-locked.
//To set nothing, use empty curly brackets "{}".
//levelup requs is a seperate key/value list {key,value} with the following keys:
//credits: credits to buy this rank
//time: how long in the rank to to get this rank
//pre: ranks required in order to get this rank
//Faction commands: comma seperated curly bracketed list "{cmd1,cmd2}" of faction commands to grant.
//This directly uses the lmd command system, so all faction commands are supported.
rank: <name> <hiarchy level> <flags> <levelup reqs> <faction command auths>

//IDEA: coffers!  Allow custom defined coffers with different access premissions based on group.

//TODO: When commands become auth-aware, allow letting faction ranks to use pure admin commands on players under their faction.
*/