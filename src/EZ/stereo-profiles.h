//
// This file contains stereo-profiles which allow us to adjust
// stereo settings for specific 2SID and 3SID tunes.
//
// By default, all 2SID and 3SID tunes will be mixed to mono.
// This list provides exceptions for tunes that should be played in stereo,
// or with specific stereo settings.
//

// 2SID
{ "/MUSICIANS/B/Booker/Icu_2SID.sid", { .bass = -6.0 } },

{ "/MUSICIANS/C/Chiummo_Gaetano/Calming_Stream_2SID.sid", { .bass = 9.0 } },
{ "/MUSICIANS/C/Chiummo_Gaetano/A_Childhood_Dream_2SID.sid", { .width = mid, .bass = 2.0 } },

{ "/MUSICIANS/E/Et1999cc/Talking_in_Your_Sleep_2SID.sid", { .bass = -3.0 } },

{ "/MUSICIANS/H/Hannula_Antti/Dead_Lock_2SID.sid", { .bass = -3.0 } },
{ "/MUSICIANS/H/Hannula_Antti/Sad_Song_2SID.sid", { .bass = 3.0 } },
{ "/MUSICIANS/H/Hannula_Antti/Electric_City_2SID.sid", { .width = narrow, .bass = -2.0 } },

{ "/MUSICIANS/H/Hoffmann_Michal/Nie_Ogolilem_Intencjonalnie_Rowa_2SID.sid", { .bass = -3.0 } },
{ "/MUSICIANS/M/Manganoid/For_Denise_2SID.sid", { .bass = -2.0 } },
{ "/MUSICIANS/M/Moraff/Lintu_ja_lapsi_2SID.sid", { .width = wide } },

{ "/MUSICIANS/J/Jammer/Stars_Are_Us_2SID.sid", { .width = wider } },
{ "/MUSICIANS/J/Jammer/Gliding_Gladly_2SID.sid", { .width = wider } },
{ "/MUSICIANS/J/Jammer/Boileroom_2SID.sid", { .width = wider } },
{ "/MUSICIANS/J/Jammer/Antipop_2SID.sid", { .width = wider } },

{ "/MUSICIANS/P/Proton/X-Files_Theme_Remix_2SID.sid", { .width = narrow } },
{ "/MUSICIANS/P/Proton/Le_Parc_Streethawk_theme_2SID.sid", { .width = narrow } },
{ "/MUSICIANS/P/Proton/Galwaymulation_2SID.sid", { .width = mid } },
{ "/MUSICIANS/P/Proton/Error-40_2SID.sid", { .width = mid } },
{ "/MUSICIANS/P/Proton/Deserted_Cities_2SID.sid", { .width = narrow, .bass = 6.0 } },
{ "/MUSICIANS/P/Proton/Airwolf_Theme_2SID.sid", { .width = mid } },
{ "/MUSICIANS/P/Proton/Saunasolmuhumppa_2SID.sid", { .width = mid } },
{ "/MUSICIANS/P/Proton/Mindwarped_2SID.sid", { .width = narrow, .bass = 4.0 } },

// Same settings, so when rendering all four files to audio and then mixing, everything matches
{ "/MUSICIANS/L/LMan/Tuneful_Eight_tune_1_2SID.sid", { .width = full, .bass = -4.0 } },
{ "/MUSICIANS/L/LMan/Tuneful_Eight_tune_2_2SID.sid", { .width = full, .bass = -4.0 } },
{ "/MUSICIANS/L/LMan/Tuneful_Eight_tune_3_2SID.sid", { .width = full, .bass = -4.0 } },
{ "/MUSICIANS/L/LMan/Tuneful_Eight_tune_4_2SID.sid", { .width = full, .bass = -4.0 } },

{ "/MUSICIANS/S/Shogoon/A_Song_for_You_2SID.sid", { .width = mid, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Cowboys_Dream_2SID.sid", { .width = mid, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/First_Chords_2SID.sid", { .width = mid, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Joe_2SID.sid", { .width = mid, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Lullaby_for_Teddy_Bear_2SID.sid", { .width = narrow } },
{ "/MUSICIANS/S/Shogoon/Lullaby_of_the_Moment_2SID.sid", { .width = wide, .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Wild_West_Weekly_2SID.sid", { .bass = 6.0 } },
{ "/MUSICIANS/S/Shogoon/Zorro_2SID.sid", { .width = full, .bass = 3.0 } },
{ "/MUSICIANS/S/Shogoon/Prelude_and_Fugato_2SID.sid", { .width = wide, .bass = 6.0 } },

{ "/MUSICIANS/U/Uctumi/Souls_Lost_to_Poseidon_2SID.sid", { .width = mid, .bass = -6.0 } },

{ "/MUSICIANS/H/Hermit/Song_of_the_Second_Moon_Delta_2SID.sid", { .width = mid, .bass = -2.0 } },

{ "/MUSICIANS/T/TheK/You_2SID.sid", { .width = wide, .bass = 6.0 } },
{ "/MUSICIANS/M/MCH/Monty_on_the_Run_D_n_B_edit_2SID.sid", { .bass = -2.0 } },

{ "/MUSICIANS/M/Mermaid/Hyperdrive_Splorf_stereo_mix_2SID.sid", { .width = narrow, .bass = 3.0 } },

// 3SID
{ "/MUSICIANS/H/Hermit/3SID_Tracker_Demo_1_3SID.sid", { .width = full } },
{ "/MUSICIANS/H/Hermit/Tree_Angel_3SID.sid", { .width = full, .bass = 3.0 } },
{ "/MUSICIANS/J/Jammer/Light_Years_3SID.sid", { .width = full, .bass = 3.0 } },
{ "/MUSICIANS/J/Jammer/Melbourne_Shuffle_3SID.sid", { .width = full } },
{ "/MUSICIANS/S/Shogoon/Cheezzy_Top_3SID.sid", { .width = narrow, .bass = -3.0 } },
{ "/MUSICIANS/S/Shogoon/Cheezzy_Top_Fixed_3SID.sid", { .width = narrow, .bass = -3.0 } },
{ "/MUSICIANS/S/Shogoon/Do_Nothing_for_3_Minutes_3SID.sid", { .width = full, .bass = 6.0 } },
