#include "p2p_messenger.h"

static int identity_key_store_get_key(signal_buffer **public_data, signal_buffer **private_data, const signal_protocol_address *address, void *user_data) {
    return SG_ERR_UNKNOWN;
}

static int identity_key_store_get_local_key(signal_buffer **public_data, signal_buffer **private_data, void *user_data) {
    p2p_network_t *net = user_data;
    ec_key_pair *pair;
    curve_generate_key_pair(net->global_context, &pair);
    *public_data = ec_public_key_serialize(ec_key_pair_get_public(pair));
    *private_data = ec_private_key_serialize(ec_key_pair_get_private(pair));
    SIGNAL_UNREF(pair);
    return 0;
}

static int session_store_load_session(signal_buffer **record, const signal_protocol_address *address, void *user_data) {
    *record = 0;
    return 0;
}

static int session_store_store_session(const signal_protocol_address *address, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

static int pre_key_store_load_pre_key(signal_buffer **record, uint32_t pre_key_id, void *user_data) {
    *record = 0;
    return SG_ERR_INVALID_KEY_ID;
}

static int pre_key_store_store_pre_key(uint32_t pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

static int signed_pre_key_store_load_signed_pre_key(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data) {
    *record = 0;
    return SG_ERR_INVALID_KEY_ID;
}

static int signed_pre_key_store_store_signed_pre_key(uint32_t signed_pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

int init_signal_store(p2p_network_t *net) {
    signal_protocol_store_context_create(&net->store, net->global_context);
    signal_protocol_store_context_set_session_store(net->store, session_store_load_session, session_store_store_session, 0, net);
    signal_protocol_store_context_set_pre_key_store(net->store, pre_key_store_load_pre_key, pre_key_store_store_pre_key, 0, net);
    signal_protocol_store_context_set_signed_pre_key_store(net->store, signed_pre_key_store_load_signed_pre_key, signed_pre_key_store_store_signed_pre_key, 0, net);
    signal_protocol_store_context_set_identity_key_store(net->store, identity_key_store_get_key, identity_key_store_get_local_key, 0, net);
    return 0;
}