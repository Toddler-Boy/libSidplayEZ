//
// This file contains stereo-profiles which allow us to adjust
// stereo settings for specific 2SID and 3SID tunes.
//
// By default, all 2SID and 3SID tunes will be mixed to mono.
// This list provides exceptions for tunes that should be played in stereo,
// or with specific stereo settings.
//

// 2SID
{ "/MUSICIANS/H/Hannula_Antti/Dead_Lock_2SID.sid", { .bass = -3.0 } },
{ "/MUSICIANS/M/Manganoid/For_Denise_2SID.sid", { .bass = -2.0 } },
{ "/MUSICIANS/E/Et1999cc/Talking_in_Your_Sleep_2SID.sid", { .bass = -3.0 } },
{ "/MUSICIANS/S/Shogoon/Zorro_2SID.sid", { .width = full, .bass = 3.0 } },
{ "/MUSICIANS/T/TheK/You_2SID.sid", { .width = wide, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Lullaby_of_the_Moment_2SID.sid", { .width = wide, .bass = 6.0 } },
{ "/MUSICIANS/M/Moraff/Lintu_ja_lapsi_2SID.sid", { .width = wide } },

{ "/MUSICIANS/S/Shogoon/First_Chords_2SID.sid", { .width = mid, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Cowboys_Dream_2SID.sid", { .width = mid, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Joe_2SID.sid", { .width = mid, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Wild_West_Weekly_2SID.sid", { .bass = 6.0 } },

// 3SID
{ "/MUSICIANS/H/Hermit/3SID_Tracker_Demo_1_3SID.sid", { .width = full } },
{ "/MUSICIANS/H/Hermit/Tree_Angel_3SID.sid", { .width = full, .bass = 3.0 } },
{ "/MUSICIANS/J/Jammer/Light_Years_3SID.sid", { .width = full, .bass = 3.0 } },
{ "/MUSICIANS/S/Shogoon/Do_Nothing_for_3_Minutes_3SID.sid", { .width = full, .bass = 6.0 } },
{ "/MUSICIANS/J/Jammer/Melbourne_Shuffle_3SID.sid", { .width = full } },
{ "/MUSICIANS/S/Shogoon/Cheezzy_Top_3SID.sid", { .width = narrow, .bass = -3.0 } },
{ "/MUSICIANS/S/Shogoon/Cheezzy_Top_Fixed_3SID.sid", { .width = narrow, .bass = -3.0 } },
