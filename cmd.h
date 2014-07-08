#ifndef _CMD_C_INCLUDED

/*
 * Starts processing the specified key maps in the background
 * 
 * NOTE Uses fixed addresses from 0xFDFD to 0xFF01 (260 bytes) which are unusable for other purposes!
 */
extern void cmd_init(uchar *a_keys_map, uchar *a_toggle_keys_map);

/*
 * Gets the next command from queue
 */
extern uchar cmds_get_next();uchar cmd_toggle_is_enabled(uchar cmd_t);

/*
 * Sets the state of a toggleable cmd
 */
extern void cmd_toggle_set(uchar cmd_t, uchar enable);

/*
 * Sets the snapshot to current value of toggle bit state for cmd_t. 
 * Returns non-zero if the state changed since last update.
 */
extern uchar cmd_toggle_snapshot_update(struct t_cmd_toggle_snapshot *snapshot, uchar cmd_t);

#endif // _CMD_C_INCLUDED

#define MAX_CMDS 16
#define CMDS_SIZE_MASK 15

struct t_cmd_toggle_snapshot
{
	uchar state;
	uchar prev_state;
};


