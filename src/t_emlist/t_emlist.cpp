#include "types.h"

// Enemy list editor tool (D:/Bio4/Prog/t_emlist.cpp, module t_emlist): edits pG->emlist (the room's ESL
// enemy list) in place with a 3D cursor, saves/loads it through the host file system and re-creates the
// enemies with EmSetFromList. The per-enemy name tables come first: their strings open the object's
// .rodata, before the strings of the game headers included below.

// Name lists for the editor's bit / type / set fields, "END"-terminated (one per enemy id, shared where
// the enemies share an id family).
static const char* em02_sub_leon_flag[] = {"END"};
static const char* em02_sub_leon_type[] = {"END"};
static const char* em02_sub_leon_set[] = {"END"};
static const char* em03_sub_ashley_flag[] = {"", "END"};
static const char* em03_sub_ashley_set[] = {"END"};
static const char* em07_sub_test_flag[] = {"", "Louice", "Type A", "Type B", "END"};
static const char* em07_sub_test_set[] = {"Normal", "Corpse", "END"};
static const char* em0e_jetski_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "END"
};
static const char* em0f_ship_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "Light on", "END"
};
static const char* em0f_ship_type[] = {"R10B", "R10D", "R10E", "R10E SET", "R10E 2", "R10E 2 SET", "END"};
static const char* em0f_ship_set[] = {"Normal", "Ride", "END"};
static const char* em10_ganado_1_flag[] = {
    "MOVE START", "", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only", "Dash PL",
    "Hat2 on", "Hat on", "Roof on", "Weapon Torch", "", "Pickup waist", "", "", "Weapon Mugen",
    "Weapon Bomb", "knit2 Cap on", "Cap2 on", "Parasite on", "Glasses on", "knit Cap on", "Cap on",
    "Left Hand", "Run Start", "", "Weapon Sickle", "Weapon ChainSaw", "Weapon Axe", "Weapon Bucket",
    "Weapon Suki", "END"
};
static const char* em10_ganado_1_type[] = {
    "EM10", "----", "----", "EM15", "EM16", "----", "----", "----", "----", "----", "----", "EM11 Woman A",
    "----", "END"
};
static const char* em11_ganado_2_flag[] = {
    "MOVE START", "Hood Apron2", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only",
    "Dash PL", "Mask of Ram", "Mask of Gold", "Roof on", "Weapon Torch", "Hood 2", "Pickup waist",
    "Make up 2", "Weapon Bowgun", "Weapon Mugen", "Weapon Bomb", "Hood Apron", "Hood", "Parasite on",
    "Make up 1", "Gold Necklace", "Silver Necklace", "Left Hand", "Run Start", "Weapon Scythe", "", "",
    "Weapon M Star", "", "Shield", "END"
};
static const char* em11_ganado_2_type[] = {
    "----", "----", "----", "----", "----", "----", "----", "EM19 Evil A", "EM1A Evil B", "EM1B Evil C",
    "----", "----", "----", "END"
};
static const char* em12_ganado_1_c_flag[] = {
    "MOVE START", "", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only", "Dash PL",
    "Hat2 on", "Hat on", "Roof on", "Weapon Torch", "", "Pickup waist", "", "", "Weapon Mugen",
    "Weapon Bomb", "knit2 Cap on", "Cap2 on", "Parasite on", "Glasses on", "knit Cap on", "Cap on",
    "Left Hand", "Run Start", "", "Weapon Sickle", "", "Weapon Axe", "Weapon Bucket", "Weapon Suki", "END"
};
static const char* em12_ganado_1_c_type[] = {
    "EM10", "EM13 Young", "----", "EM15 G.papa", "EM16 C.S.", "----", "----", "----", "----", "----",
    "----", "----", "----", "END"
};
static const char* em13_ganado_1_w_flag[] = {
    "MOVE START", "", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only", "Dash PL",
    "Hat2 on", "Hat on", "Roof on", "Weapon Torch", "", "Pickup waist", "", "Weapon Bowgun",
    "Weapon Mugen", "Weapon Bomb", "knit2 Cap on", "Cap2 on", "Parasite on", "Glasses on", "knit Cap on",
    "Cap on", "Left Hand", "Run Start", "", "Weapon Sickle", "", "Weapon Axe", "Weapon Bucket",
    "Weapon Suki", "END"
};
static const char* em13_ganado_1_w_type[] = {
    "EM10", "EM13 Young", "----", "EM15 G.papa", "EM16 C.S.", "----", "EM18 Trader", "----", "----",
    "----", "----", "----", "----", "END"
};
static const char* em14_ganado_2_w_flag[] = {
    "MOVE START", "Hood Apron2", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only",
    "Dash PL", "Mask of Ram", "Mask of Gold", "Roof on", "Weapon Torch", "Hood 2", "Pickup waist",
    "Make up 2", "Weapon Bowgun", "Weapon Mugen", "Weapon Bomb", "Hood Apron", "Hood", "Parasite on",
    "Make up 1", "Gold Necklace", "Silver Necklace", "Left Hand", "Run Start", "Weapon Scythe", "", "",
    "Weapon M Star", "", "Shield", "END"
};
static const char* em14_ganado_2_w_type[] = {
    "----", "----", "----", "----", "----", "----", "EM18 Trader", "EM19 Evil A", "EM1A Evil B",
    "EM1B Evil C", "----", "----", "----", "END"
};
static const char* em15_ganado_1_c_flag[] = {
    "MOVE START", "", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only", "Dash PL",
    "Hat2 on", "Hat on", "Roof on", "Weapon Torch", "", "Pickup waist", "Weapon Knife", "", "Weapon Mugen",
    "Weapon Bomb", "Woman Hood2 on", "Cap2 on", "Parasite on", "Glasses on", "Woman Hood on", "Cap on",
    "Left Hand", "Run Start", "", "Weapon Sickle", "Weapon ChainSaw", "Weapon Axe", "Weapon Bucket",
    "Weapon Suki", "END"
};
static const char* em16_ganado_1_c2_flag[] = {
    "MOVE START", "", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only", "Dash PL",
    "Hat2 on", "Hat on", "Roof on", "Weapon Torch", "", "Pickup waist", "Weapon Knife", "", "Weapon Mugen",
    "Weapon Bomb", "Woman Hood2 on", "Cap2 on", "Parasite on", "Glasses on", "Woman Hood on", "Cap on",
    "Left Hand", "Run Start", "", "Weapon Sickle", "Weapon ChainSaw", "Weapon Axe", "", "Weapon Suki",
    "END"
};
static const char* em16_ganado_1_c2_type[] = {
    "----", "----", "----", "EM15", "EM16", "----", "----", "----", "----", "----", "----", "EM11 Woman A",
    "EM12 Woman B", "END"
};
static const char* em17_ganado_1_c2_flag[] = {
    "MOVE START", "", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only", "Dash PL",
    "Hat2 on", "Hat on", "Roof on", "Weapon Torch", "", "Pickup waist", "Weapon Knife", "", "Weapon Mugen",
    "Weapon Bomb", "Woman Hood2 on", "Cap2 on", "Parasite on", "Glasses on", "Woman Hood on", "Cap on",
    "Left Hand", "Run Start", "", "Weapon Sickle", "", "Weapon Axe", "", "Weapon Suki", "END"
};
static const char* em17_ganado_1_c2_type[] = {
    "EM10", "EM13 Young", "----", "EM15 G.papa", "----", "----", "----", "----", "----", "----", "----",
    "----", "EM12 Woman B", "END"
};
static const char* em19_ganado_2_a_flag[] = {
    "MOVE START", "Hood Apron2", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only",
    "Dash PL", "Mask of Ram", "Mask of Gold", "Roof on", "Weapon Torch", "Hood 2", "Pickup waist",
    "Make up 2", "Weapon Bowgun", "Weapon Mugen", "Weapon Bomb", "Hood Apron", "Hood", "Parasite on",
    "Make up 1", "Gold Necklace", "Silver Necklace", "Left Hand", "Run Start", "Weapon Scythe", "", "",
    "Weapon M Star", "", "Shield", "END"
};
static const char* em19_ganado_2_a_type[] = {
    "----", "----", "----", "----", "----", "----", "----", "EM19 Evil A", "EM1A Evil B", "EM1B Evil C",
    "----", "----", "----", "----", "END"
};
static const char* em1a_ganado_2_r_flag[] = {
    "MOVE START", "Hood Apron2", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only",
    "Dash PL", "Mask of Ram", "Mask of Gold", "Roof on", "Weapon Torch", "Hood 2", "Pickup waist",
    "Make up 2", "Weapon Bowgun", "Weapon Mugen", "Weapon Bomb", "Hood Apron", "Hood", "Parasite on",
    "Make up 1", "Gold Necklace", "Silver Necklace", "Left Hand", "Run Start", "Weapon Scythe", "", "",
    "Weapon M Star", "Weapon Rocket", "Shield", "END"
};
static const char* em1a_ganado_2_r_type[] = {
    "----", "----", "----", "----", "----", "----", "----", "EM19 Evil A", "EM1A Evil B", "EM1B Evil C",
    "----", "----", "----", "----", "END"
};
static const char* em1b_ganado_2_cm_flag[] = {
    "MOVE START", "Hood Apron2", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only",
    "Dash PL", "Mask of Ram", "Mask of Gold", "Roof on", "Weapon Torch", "Hood 2", "Pickup waist",
    "Make up 2", "Weapon Bowgun", "Weapon Mugen", "Weapon Bomb", "Hood Apron", "Hood", "Parasite on",
    "Make up 1", "Gold Necklace", "Silver Necklace", "Left Hand", "Run Start", "Weapon Scythe", "", "",
    "Weapon M Star", "", "Shield", "END"
};
static const char* em1b_ganado_2_cm_type[] = {
    "----", "----", "----", "----", "----", "----", "----", "EM19 Evil A", "EM1A Evil B", "----",
    "EM1C ClawMan", "----", "----", "----", "END"
};
static const char* em1c_ganado_2_cm2_type[] = {
    "----", "----", "----", "----", "----", "----", "----", "EM19 Evil A", "----", "EM1B Evil C",
    "EM1C ClawMan", "----", "----", "EM1D ClawMan 2", "END"
};
static const char* em1d_ganado_3_gm_flag[] = {
    "MOVE START", "Belt B", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only",
    "Dash PL", "Belt A", "Full Face Guard", "Roof on", "Weapon Club", "Mask C", "Pickup waist", "Mask B",
    "Weapon Bowgun", "Weapon Mugen", "Weapon Bomb", "Mask A", "Helmet B on", "Parasite on", "Helmet A on",
    "", "Cap on", "Left Hand", "Run Start", "Weapon StunRod", "Weapon Axe", "", "Weapon M Star",
    "Weapon Rocket", "Shield", "END"
};
static const char* em20_ganado_3_g_c_flag[] = {
    "MOVE START", "", "Low Power", "Lock PL", "Gunshot Musi", "Jump atk off", "Go Ashley only", "Dash PL",
    "Belt", "Full Face Guard", "Roof on", "Weapon Club", "Mask C", "Pickup waist", "Mask B",
    "Weapon Bowgun", "Weapon Mugen", "Weapon Bomb", "Mask A", "Helmet on", "Parasite on", "", "Belt C",
    "Cap on", "Left Hand", "Run Start", "Weapon StunRod", "Weapon Axe", "Weapon ChainSaw", "Weapon M Star",
    "Weapon Rocket", "Shield", "END"
};
static const char* em1d_ganado_3_gm_type[] = {
    "----", "----", "Gatling Man", "----", "----", "----", "----", "----", "----", "----", "----", "----",
    "----", "----", "EM1E Ganado A", "EM1E Ganado B", "EM1E Ganado C", "EM1E Ganado D", "EM1E Ganado A2",
    "EM1E Ganado B2", "EM1E Ganado C2", "EM1E Ganado D2", "----", "----", "----", "END"
};
static const char* em1e_ganado_3_w_type[] = {
    "----", "----", "----", "----", "----", "----", "EM18 Trader", "----", "----", "----", "----", "----",
    "----", "----", "EM1E Ganado A", "EM1E Ganado B", "----", "----", "----", "----", "----", "----",
    "----", "EM20 Gas Mask A", "----", "EM20 Gas Mask B", "END"
};
static const char* em1f_ganado_3_type[] = {
    "----", "----", "----", "----", "----", "----", "----", "----", "----", "----", "----", "----", "----",
    "----", "EM1E Ganado A", "EM1E Ganado B", "EM1E Ganado C", "----", "----", "----", "----", "----",
    "----", "----", "EM20 End of Century", "END"
};
static const char* em20_ganado_3_g_c_type[] = {
    "----", "----", "----", "----", "----", "----", "----", "----", "----", "----", "----", "----", "----",
    "----", "EM1E Ganado A", "EM1E Ganado B", "EM1E Ganado C", "EM1E Ganado D", "EM1E Ganado A2",
    "EM1E Ganado B2", "EM1E Ganado C2", "EM1E Ganado D2", "EM1F Chain Saw", "END"
};
static const char* em10_ganado_1_set[] = {
    "Normal", "----", "R100 Cliff A", "R100 Cliff B", "R100 Cliff C", "R101 Bucket A", "R101 Bucket B",
    "R101 Suki A", "R101 Suki B", "R101 Suki C", "R101 Sickle A", "R101 Cart A", "Evt:Dash start",
    "Evt:Walk start", "Walk", "Guard Left", "Guard Right", "Guard Sit", "----", "Hide", "Hide Fall",
    "Hide Jump", "Appear L", "Appear R", "Ride Truck", "Catapult mode", "R103 Bucket", "R100 Turn&Walk",
    "Rock Push", "R11C 1F IN", "R10C Parasite", "R100 Walk & Stay", "R202 Finger", "R11C 1F IN2",
    "Stay & Walk", "Attack wait", "R106 Fix Bomber", "Homing Bomber", "R204 Prayer", "R222 Dragon A",
    "R222 Dragon B", "R222 Dragon C", "R227 Barrel", "Homing Bomber2", "R10F G.Jump A", "R10F Gondola",
    "R209 Dash & Sit", "Evt:Wait Dash", "R10C ParaCancel", "R10F G.Jump B", "R11D Appear 1",
    "R11D Appear 2", "R212 Drill", "R201 Event wait", "Rocket Wait", "R21B Trolley Jump",
    "R21B Trolley Jump2", "R303 Fire Dash", "Work", "R300 Take Ashley", "R320 Gatling", "R300 Gatling",
    "R305 Bomber", "R321 Dead Body", "R408 Bomber", "END"
};
static const char* em10_ganado_1_chr[] = {"Chase", "Keep", "Rush", "Stop", "Escape", "In Room", "END"};
static const char* em18_trader_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "END"
};
static const char* em18_trader_type[] = {"Normal", "Counter", "END"};
static const char* em18_trader_set[] = {"END"};
static const char* em3c_armor_flag[] = {
    "START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "END"
};
static const char* em3c_armor_type[] = {"Gray + AXE", "Gray + SWORD", "Black + AXE", "Black + SWORD", "END"};
static const char* em3c_armor_set[] = {"Normal", "Start Wait", "Atk Wait", "END"};
static const char* em29_bat_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "END"
};
static const char* em29_bat_type[] = {"END"};
static const char* em29_bat_set[] = {"Land", "Ceiling", "END"};
static const char* em2a_trap_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "END"
};
static const char* em2a_trap_type[] = {"Trap1 Hasami", "Trap2 P.E. Type A", "Trap2 P.E. Type B", "END"};
static const char* em2a_trap_set[] = {"Normal", "R100 Dog trap1", "Break", "END"};
static const char* em2b_elgigante_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "Route around", "END"
};
static const char* em2b_elgigante_type[] = {"Type A", "Type Mask", "Type Handsome", "Type Blue", "END"};
static const char* em2b_elgigante_set[] = {"Normal", "From Event", "R11E Appear", "R224 Cage Wait", "END"};
static const char* em2c_insectboss_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "END"
};
static const char* em2c_insectboss_type[] = {"Normal", "Tail", "END"};
static const char* em2c_insectboss_set[] = {"Normal", "Ceiling", "Re set", "Tail Hide", "END"};
static const char* em2d_insecthuman_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "No reset", "No wall", "No Camouflage", "END"
};
static const char* em2d_insecthuman_type[] = {
    "Normal", "Normal+Wing", "White", "White+Wing", "Simple+Wing", "Boss", "END"
};
static const char* em2d_insecthuman_set[] = {
    "Normal", "Air", "Ceiling", "Run start", "R213 Nest Wait", "Debug wall", "END"
};
static const char* em2e_spider_sml_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em2e_spider_sml_type[] = {"Normal", "White", "END"};
static const char* em2e_spider_sml_set[] = {"Normal", "Wall", "END"};
static const char* em2f_salamander_flag[] = {
    "MOVE START", "FIND ENEMY", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "END"
};
static const char* em2f_salamander_type[] = {"Type A", "Type B", "END"};
static const char* em2f_salamander_set[] = {"Wait", "Swim", "Critical", "END"};
static const char* em30_saddler_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "Parasite on", "END"
};
static const char* em30_saddler_type[] = {"Hood on", "Hood off", "END"};
static const char* em30_saddler_set[] = {"END"};
static const char* em31_saddler2_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em31_saddler2_type[] = {"Body", "Tentacle", "END"};
static const char* em31_saddler2_set[] = {"END"};
static const char* em32_u_3_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "Change Type3", "Change Type2", "END"
};
static const char* em32_u_3_type[] = {"END"};
static const char* em32_u_3_set[] = {"END"};
static const char* em34_no_1_no_2_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em34_no_1_no_2_type[] = {
    "No.1 Mayor", "No.2 Salazar", "Insect Boss 1", "Insect Boss 2", "END"
};
static const char* em34_no_1_no_2_set[] = {"END"};
static const char* em35_no_1_after_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em35_no_1_after_type[] = {"Normal", "Upper", "Lower", "END"};
static const char* em35_no_1_after_set[] = {"Normal", "Divide", "END"};
static const char* em36_regenerater_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "Weak Set", "Weak 5", "Weak 4", "Weak 3", "Weak 2", "Weak 1", "END"
};
static const char* em36_regenerater_type[] = {"Normal A", "Normal B", "Strong A", "Strong B", "END"};
static const char* em36_regenerater_set[] = {
    "Normal", "R307 Bed", "R309 Appear", "R308 Appear", "R310 Appear", "END"
};
static const char* em38_no_2_after_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em38_no_2_after_type[] = {"Body", "Tentacle A", "Tentacle B", "Upper", "Lower", "END"};
static const char* em38_no_2_after_set[] = {"END"};
static const char* em39_no_3_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em39_no_3_type[] = {"Type 1", "Type 2", "Type 3", "END"};
static const char* em39_no_3_set[] = {"Normal", "Test", "Success", "Failure", "Normal2", "END"};
static const char* em3a_seeker_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em3a_seeker_type[] = {"Machine Gun", "Missile", "Bomb Robbot", "END"};
static const char* em3a_seeker_set[] = {
    "Normal", "Up", "Down", "Right", "Left", "Forward", "Bomb Ground", "Hide2", "END"
};
static const char* em3b_truck_cart_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em3b_truck_cart_type[] = {"Truck", "Cart", "Stop Cart", "END"};
static const char* em3b_truck_cart_set[] = {"END"};
static const char* em21_dog_flag[] = {"END"};
static const char* em21_dog_type[] = {"TYPE0", "TYPE1", "END"};
static const char* em21_dog_set[] = {"Normal", "R100 Trap", "VS Elgigante", "END"};
static const char* em22_enemy_dog_flag[] = {
    "MOVE START", "FIND ENEMY", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "Parasite on", "No around", "END"
};
static const char* em22_enemy_dog_type[] = {"END"};
static const char* em22_enemy_dog_set[] = {
    "Normal", "R11B Set A", "R11B Set B", "R11B Set C", "In Cage", "Jump Wait", "END"
};
static const char* em23_crow_flag[] = {"END"};
static const char* em23_crow_type[] = {"END"};
static const char* em23_crow_set[] = {"Normal", "R20A Landing", "END"};
static const char* em24_snake_s_flag[] = {"END"};
static const char* em24_snake_s_type[] = {"END"};
static const char* em24_snake_s_set[] = {"Box wait", "Coil wait", "END"};
static const char* em25_parasite_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "END"
};
static const char* em25_parasite_type[] = {"END"};
static const char* em25_parasite_set[] = {"Hide", "Wait", "END"};
static const char* em26_cow_flag[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "END"
};
static const char* em26_cow_type[] = {"Type1", "Type2", "END"};
static const char* em26_cow_set[] = {"END"};
static const char* em27_blackbass_flag[] = {
    "MOVE START", "FIND ENEMY", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "END"
};
static const char* em27_blackbass_type[] = {"Normal", "Super Black Bass", "END"};
static const char* em27_blackbass_set[] = {"END"};
static const char* em28_chicken_flag[] = {
    "MOVE START", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "No Egg", "END"
};
static const char* em28_chicken_type[] = {"Type 1", "Type 2", "END"};
static const char* em28_chicken_set[] = {"END"};

// Per enemy id: display name and the name lists of its em flags, type, set and character fields.
struct EmListIdInfo {
    char name[16];
    const char** flag;
    const char** type;
    const char** set;
    const char** chr;
};

static EmListIdInfo EmListIdTbl[64] = {
    {"Player 0", NULL, NULL, NULL, NULL},
    {"Sub ", NULL, NULL, NULL, NULL},
    {"Sub Leon", em02_sub_leon_flag, em02_sub_leon_type, em02_sub_leon_set, NULL},
    {"Sub Ashley", em03_sub_ashley_flag, em03_sub_ashley_flag + 1, em03_sub_ashley_set, NULL},
    {"Sub Luis", NULL, NULL, NULL, NULL},
    {"Sub 5", NULL, NULL, NULL, NULL},
    {"Sub 6", NULL, NULL, NULL, NULL},
    {"Sub test", em07_sub_test_flag, em07_sub_test_flag + 1, em07_sub_test_set, NULL},
    {"Sub 8", NULL, NULL, NULL, NULL},
    {"Sub 9", NULL, NULL, NULL, NULL},
    {"Sub a", NULL, NULL, NULL, NULL},
    {"Sub b", NULL, NULL, NULL, NULL},
    {"Sub c", NULL, NULL, NULL, NULL},
    {"Sub d", NULL, NULL, NULL, NULL},
    {"JetSki", em0e_jetski_flag, NULL, NULL, NULL},
    {"Ship", em0f_ship_flag, em0f_ship_type, em0f_ship_set, NULL},
    {"Ganado 1", em10_ganado_1_flag, em10_ganado_1_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 2", em11_ganado_2_flag, em11_ganado_2_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 1-c", em12_ganado_1_c_flag, em12_ganado_1_c_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 1+w", em13_ganado_1_w_flag, em13_ganado_1_w_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 2+w", em14_ganado_2_w_flag, em14_ganado_2_w_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 1+c", em15_ganado_1_c_flag, em10_ganado_1_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 1+c2", em16_ganado_1_c2_flag, em16_ganado_1_c2_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 1-c2", em17_ganado_1_c2_flag, em17_ganado_1_c2_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Trader", em18_trader_flag, em18_trader_type, em18_trader_set, NULL},
    {"Ganado 2+A", em19_ganado_2_a_flag, em19_ganado_2_a_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 2+R", em1a_ganado_2_r_flag, em1a_ganado_2_r_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 2+cm", em1b_ganado_2_cm_flag, em1b_ganado_2_cm_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 2+cm2", em1b_ganado_2_cm_flag, em1c_ganado_2_cm2_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 3+GM", em1d_ganado_3_gm_flag, em1d_ganado_3_gm_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 3+W", em1d_ganado_3_gm_flag, em1e_ganado_3_w_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 3", em1d_ganado_3_gm_flag, em1f_ganado_3_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Ganado 3+G+C", em20_ganado_3_g_c_flag, em20_ganado_3_g_c_type, em10_ganado_1_set, em10_ganado_1_chr},
    {"Dog", em21_dog_flag, em21_dog_type, em21_dog_set, NULL},
    {"Enemy Dog", em22_enemy_dog_flag, em22_enemy_dog_type, em22_enemy_dog_set, NULL},
    {"Crow", em23_crow_flag, em23_crow_type, em23_crow_set, NULL},
    {"Snake S", em24_snake_s_flag, em24_snake_s_type, em24_snake_s_set, NULL},
    {"Parasite", em25_parasite_flag, em25_parasite_type, em25_parasite_set, NULL},
    {"Cow", em26_cow_flag, em26_cow_type, em26_cow_set, NULL},
    {"BlackBass", em27_blackbass_flag, em27_blackbass_type, em27_blackbass_set, NULL},
    {"Chicken", em28_chicken_flag, em28_chicken_type, em28_chicken_set, NULL},
    {"Bat", em29_bat_flag, em29_bat_type, em29_bat_set, NULL},
    {"Trap", em2a_trap_flag, em2a_trap_type, em2a_trap_set, NULL},
    {"Elgigante", em2b_elgigante_flag, em2b_elgigante_type, em2b_elgigante_set, NULL},
    {"InsectBoss", em2c_insectboss_flag, em2c_insectboss_type, em2c_insectboss_set, NULL},
    {"InsectHuman", em2d_insecthuman_flag, em2d_insecthuman_type, em2d_insecthuman_set, NULL},
    {"Spider sml", em2e_spider_sml_flag, em2e_spider_sml_type, em2e_spider_sml_set, NULL},
    {"Salamander", em2f_salamander_flag, em2f_salamander_type, em2f_salamander_set, NULL},
    {"Saddler", em30_saddler_flag, em30_saddler_type, em30_saddler_set, NULL},
    {"Saddler2", em31_saddler2_flag, em31_saddler2_type, em31_saddler2_set, NULL},
    {"U 3", em32_u_3_flag, em32_u_3_type, em32_u_3_set, NULL},
    {"------", NULL, NULL, NULL, NULL},
    {"No.1 & No.2", em34_no_1_no_2_flag, em34_no_1_no_2_type, em34_no_1_no_2_set, NULL},
    {"No.1 After", em35_no_1_after_flag, em35_no_1_after_type, em35_no_1_after_set, NULL},
    {"Regenerater", em36_regenerater_flag, em36_regenerater_type, em36_regenerater_set, NULL},
    {"------", NULL, NULL, NULL, NULL},
    {"No.2 After", em38_no_2_after_flag, em38_no_2_after_type, em38_no_2_after_set, NULL},
    {"No.3", em39_no_3_flag, em39_no_3_type, em39_no_3_set, NULL},
    {"Seeker", em3a_seeker_flag, em3a_seeker_type, em3a_seeker_set, NULL},
    {"Truck&Cart", em3b_truck_cart_flag, em3b_truck_cart_type, em3b_truck_cart_set, NULL},
    {"Armor", em3c_armor_flag, em3c_armor_type, em3c_armor_set, NULL},
    {"Helicopter", NULL, NULL, NULL, NULL},
    {"r22c Mark", NULL, NULL, NULL, NULL},
    {"", NULL, NULL, NULL, NULL},
};

#include "light.h"
#include "atari.h"
#include "em.h"
#include "em_set.h"
#include "global.h"
#include "joy.h"
#include "eprintf.h"
#include "main_sub.h"
#include "main_mem.h"
#include "scheduler.h"
#include "camera.h"
#include "file.h"
#include "t_prim.h"
#include "t_util.h"

extern "C" int sprintf(char* buf, const char* fmt, ...);
extern "C" void memclr_asm(void* p, u32 size);

// Editor state (0x3E0 bytes, Debug_alloc'd by emlist_init).
struct EmListWork {
    int routine;       // 0x00  index into EmList.routine
    int step;          // 0x04
    int x8;            // 0x08
    int xC;            // 0x0C
    int listNo;        // 0x10  entry of pG->emlist being edited
    int x14;           // 0x14
    u8 pad_18[8];
    int fileNo;        // 0x20  file number + 1 of the last save/load
    u8 pad_24[0x24];
    f32 cursorX;       // 0x48  screen cursor
    f32 cursorY;       // 0x4C
    int x50;           // 0x50
    u8 id;             // 0x54  enemy id selected in the id menu
    u8 idNum;          // 0x55  number of named ids in EmListIdTbl
    u8 pad_56[6];
    EmListData cur;    // 0x5C  entry being edited
    int x7C;           // 0x7C
    u8 pad_80[0x178 - 0x80];
    JOY joy;           // 0x178
};

static void emlist_r0_main();
static void emlist_r0_target();
static void emlist_r0_set_id();
static void emlist_r0_set_room();
static void emlist_r0_set_pos();
static void emlist_r0_set_ang();
static void emlist_r0_set_be_flag();
static void emlist_r0_set_type();
static void emlist_r0_set_set();
static void emlist_r0_set_em_flag();
static void emlist_r0_set_char();
static void emlist_r0_set_hp();
static void emlist_r0_set_guard_r();
static void emlist_r0_menu();
static void emlist_r0_save();
static void emlist_r0_load();
static void emlist_r0_clear();
static void emlist_r0_sort();
static void emlist_r0_set_exit();

// The work pointer is a member of the control block: every store through it reloads the pointer.
struct EmListCtrl {
    EmListWork* wk;
    void (*routine[19])();
};

static EmListCtrl EmList = {
    NULL,
    {
        emlist_r0_main,
        emlist_r0_target,
        emlist_r0_set_id,
        emlist_r0_set_room,
        emlist_r0_set_pos,
        emlist_r0_set_ang,
        emlist_r0_set_be_flag,
        emlist_r0_set_type,
        emlist_r0_set_set,
        emlist_r0_set_em_flag,
        emlist_r0_set_char,
        emlist_r0_set_hp,
        emlist_r0_set_guard_r,
        emlist_r0_menu,
        emlist_r0_save,
        emlist_r0_load,
        emlist_r0_clear,
        emlist_r0_sort,
        emlist_r0_set_exit,
    },
};

static char main_menu[7][0x80] = {
    "LIST      ",
    "SORT  LIST      ",
    "CLEAR LIST     ",
    "SET AND EXIT   (Don't set already set)",
    "LOAD      ",
    "SAVE      ",
    "EXIT      ",
};

static char target_menu[13][0x10] = {
    "ID      ", "ROOM    ", "POS     ", "ANG     ", "BE_FLAG ", "TYPE    ", "SET     ",
    "EM FLAG ", "CHARA   ", "HP      ", "Guard R ", "Set and Exit", "EXIT    ",
};

static char be_flag_name[8][0x10] = {"DIE", "", "", "", "", "", "SET", "ALIVE"};

static GXColor list_color[2] = {{0x80, 0x00, 0x00, 0xFF}, {0xFF, 0x40, 0x40, 0xFF}};

void emlist_init();
void emlist_exit();
void emlist_select_id();
void emlist_main_disp();
void emlist_target_help_disp();
void emlist_target_disp(int flag);
void emlist_target_menu_disp();
void emlist_menu_disp();
void emlist_file_menu_disp();
void emlist_yes_no_menu_disp();
void emlist_select_id_disp();
void emlist_set_room_disp();
void emlist_set_pos_disp();
void emlist_set_ang_disp();
void emlist_set_be_flag_disp();
void emlist_set_type_disp();
void emlist_set_set_disp();
void emlist_set_em_flag_disp();
void emlist_set_char_disp();
void emlist_set_hp_disp();
void emlist_set_guard_r_disp();
void emlist_file_save(int no);
int emlist_file_load(int no);
void emlist_set_fname(char* buf, int no, int mode);
void emlist_EmDir_disp();
void emlist_catch_em();
void emlist_em_move_to_cursor();
int emlist_get_numof_str(const char** tbl);
void emlistCameraMove();
void emlistCamToPoin();
void emlistCursorToTarget();

void ToolEmList()
{
    emlist_init();
    for (;;) {
        emlistCameraMove();
        EmList.routine[EmList.wk->routine]();
        emlist_EmDir_disp();
        LightMgr.move();
        if (pG->flags_60 & 0x40000000) {
            if ((int) pG->flags_500C >= 0) {
                pG->flags_500C |= 0x80000000;
            }
            SatMgr.disp(0);
        }
        CameraMove();
        TaskSleep(1);
    }
}

void emlist_init()
{
    u32 i;

    TaskSuspend(0);
    TaskSleep(1);
    TutilInitDefault();
    TOOL_FLAG(OFS_STOP_FLG) |= 0x200000;
    TOOL_FLAG(OFS_DISP_FLG) |= 0x1000000;
    TOOL_FLAG(OFS_DISP_FLG) |= 0x800000;
    TOOL_FLAG(OFS_DEBUG_FLG) |= 0x80000000;
    TOOL_FLAG(OFS_DEBUG_FLG) |= 0x20000000;
    TOOL_FLAG(OFS_STOP_FLG) |= 0x800000;
    TOOL_FLAG(OFS_DEBUG_FLG) |= 0x10000000;
    EmList.wk = (EmListWork*) Debug_alloc(sizeof(EmListWork), 1);
    if (EmList.wk == NULL) {
        for (i = 0; i < 90; i++) {
            eprintf(100, 100, 0, 0, "MEMORY ALLOCATE ERROR");
            TaskSleep(1);
        }
        emlist_exit();
    }
    EmList.wk->fileNo = 0;
    EmList.wk->cursorX = (Screen.x + Screen.width) * 0.5f;
    EmList.wk->cursorY = (Screen.y + Screen.height) * 0.5f;
    EmList.wk->idNum = 0;
    for (i = 0; i < 64; i++) {
        if (EmListIdTbl[i].name[0] == 0) {
            break;
        }
        EmList.wk->idNum++;
    }
    EmList.wk->x7C = 0;
    EmList.wk->id = 0x15;
    EmList.wk->cur.id = 0x15;
    EmList.wk->cur.id = 0;
    EmList.wk->cur.type = 0;
    EmList.wk->cur.x3 = 0;
    EmList.wk->cur.flags4 = 0;
    EmList.wk->cur.xB = 0;
    EmList.wk->cur.x1A = 10;
    EmList.wk->cur.hp = 1000;
    EmList.wk->cur.pad_A = 0;
    EmList.wk->routine = 1;
    EmList.wk->step = 0;
    EmList.wk->x8 = 0;
    EmList.wk->xC = 0;
    EmList.wk->x14 = 0;
    EmList.wk->x50 = -1;
    EmList.wk->step = 1;
    EmList.wk->cursorX = (Screen.x + Screen.width) * 0.5f;
    EmList.wk->cursorY = (Screen.y + Screen.height) * 0.5f;
    EmList.wk->listNo = 0;
}

void emlist_exit()
{
    TOOL_FLAG(OFS_STOP_FLG) &= ~0x200000;
    TOOL_FLAG(OFS_DISP_FLG) &= ~0x1000000;
    TOOL_FLAG(OFS_DISP_FLG) &= ~0x800000;
    TOOL_FLAG(OFS_DEBUG_FLG) &= ~0x80000000;
    TOOL_FLAG(OFS_STOP_FLG) &= ~0x800000;
    TOOL_FLAG(OFS_DEBUG_FLG) &= ~0x10000000;
    TutilQuitDefault();
    TaskSignal(0);
    TaskExit();
}

static void emlist_r0_main()
{
}

static void emlist_r0_target()
{
}

static void emlist_r0_set_id()
{
    EmListData* p = EM_LIST(EmList.wk->listNo);

    emlist_select_id();
    if (EmList.wk->joy.trg & JOY_A) {
        p->id = EmList.wk->id;
        EmList.wk->cur = *p;
        EmList.wk->routine = 1;
        EmList.wk->step = 0;
        EmList.wk->x8 = 0;
        EmList.wk->xC = 0;
    }
    if (EmList.wk->joy.trg & JOY_B) {
        EmList.wk->routine = 1;
        EmList.wk->step = 0;
        EmList.wk->x8 = 0;
        EmList.wk->xC = 0;
    }
    emlist_select_id_disp();
    emlist_target_disp(1);
}

// Moves the id-menu cursor: up/down by one, left/right by a column of 16.
void emlist_select_id()
{
    if (EmList.wk->joy.rep2 & (JOY_UP | 0x80000)) {
        EmList.wk->id--;
        if (EmList.wk->id <= 1) {
            EmList.wk->id = 2;
        }
    }
    if (EmList.wk->joy.rep2 & (JOY_DOWN | 0x40000)) {
        EmList.wk->id++;
        if (EmList.wk->id >= EmList.wk->idNum) {
            EmList.wk->id = EmList.wk->idNum - 1;
        }
    }
    if (EmList.wk->joy.rep2 & (JOY_LEFT | 0x10000)) {
        if (EmList.wk->id > 0x11) {
            EmList.wk->id -= 0x10;
        }
    }
    if (EmList.wk->joy.rep2 & (JOY_RIGHT | 0x20000)) {
        if (EmList.wk->id < EmList.wk->idNum - 0x10) {
            EmList.wk->id += 0x10;
        }
    }
}

static void emlist_r0_set_room()
{
}

static void emlist_r0_set_pos()
{
}

static void emlist_r0_set_ang()
{
}

static void emlist_r0_set_be_flag()
{
}

static void emlist_r0_set_type()
{
}

static void emlist_r0_set_set()
{
}

static void emlist_r0_set_em_flag()
{
}

static void emlist_r0_set_char()
{
}

static void emlist_r0_set_hp()
{
}

static void emlist_r0_set_guard_r()
{
}

static void emlist_r0_menu()
{
}

static void emlist_r0_save()
{
}

static void emlist_r0_load()
{
}

static void emlist_r0_clear()
{
}

static void emlist_r0_sort()
{
}

// Clears the death bits of the current list and re-creates its enemies.
static void emlist_r0_set_exit()
{
    int i;

    for (i = 0; i < 255; i++) {
        if (pG->emlist_no >= 0) {
            pG->em_dead[pG->emlist_no][i >> 5] &= ~(0x80000000 >> (i & 0x1F));
        }
    }
    EmSetFromList();
    emlist_exit();
}

void emlist_main_disp()
{
}

void emlist_target_help_disp()
{
}

void emlist_target_disp(int flag)
{
}

void emlist_target_menu_disp()
{
}

void emlist_menu_disp()
{
}

void emlist_file_menu_disp()
{
}

void emlist_yes_no_menu_disp()
{
}

void emlist_select_id_disp()
{
}

void emlist_set_room_disp()
{
}

void emlist_set_pos_disp()
{
}

void emlist_set_ang_disp()
{
}

void emlist_set_be_flag_disp()
{
}

void emlist_set_type_disp()
{
}

void emlist_set_set_disp()
{
}

void emlist_set_em_flag_disp()
{
}

void emlist_set_char_disp()
{
}

void emlist_set_hp_disp()
{
}

void emlist_set_guard_r_disp()
{
}

// Writes the list to both hosts.
void emlist_file_save(int no)
{
    char name[0x100];

    EmList.wk->fileNo = no + 1;
    emlist_set_fname(name, no, 0);
    HDWrite(name, pG->emlist, 0x1FE0);
    emlist_set_fname(name, no, 1);
    HDWrite(name, pG->emlist, 0x1FE0);
}

int emlist_file_load(int no)
{
    char name[0x100];
    int ret;

    EmList.wk->fileNo = no + 1;
    emlist_set_fname(name, no, 0);
    ret = HDRead(name, pG->emlist);
    if (ret == 0) {
        memclr_asm(pG->emlist, 0x1FE0);
        return 0;
    }
    return ret;
}

// "<host><dir>/emlist%02x.esl": file `no` = dir no / 10, index no % 10.
void emlist_set_fname(char* buf, int no, int mode)
{
    char dir[3][0x20] = {"/data/etc/emlist", "/data/etc/emleon", "/data/etc/omake"};

    switch (mode) {
    case 0:
    default:
        sprintf(buf, "%s%s%02x.esl", "x:\\soft", dir[no / 10], no % 10);
        break;
    case 1:
        sprintf(buf, "%s%s%02x.esl", "d:\\bio4", dir[no / 10], no % 10);
        break;
    }
}

void emlist_EmDir_disp()
{
}

void emlist_catch_em()
{
}

void emlist_em_move_to_cursor()
{
}

// Number of entries before the "END" terminator (0 without a table or terminator).
int emlist_get_numof_str(const char** tbl)
{
    int i;

    if (tbl == NULL) {
        return 0;
    }
    for (i = 0; i < 256; i++) {
        if (tbl[i][0] == 'E' && tbl[i][1] == 'N' && tbl[i][2] == 'D') {
            return i;
        }
    }
    return 0;
}

void emlistCameraMove()
{
}

void emlistCamToPoin()
{
}

void emlistCursorToTarget()
{
}
