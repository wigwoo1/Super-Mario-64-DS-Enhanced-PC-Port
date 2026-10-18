/* The ROM global has C++ linkage in actor constructors and is emitted as a
   null host placeholder by pc_link_data.cpp. Point it at the native profile
   table before any actor constructor reads actorID -> profile. */
extern void *data_020a4bb8;
extern "C" void *data_020a4bb8_storage[];

extern "C" void pc_sync_actor_profile_table_cpp(void)
{
    data_020a4bb8 = data_020a4bb8_storage;
}
