#include <stdlib.h>
#include <string.h>

#include <landru/file.h>
#include <landru/res.h>
#include <landru/stream.h>

/* Synchronous stream interface backed by whole-file memory buffers. */

typedef struct {
	char filename[LANDRU_STREAM_FILENAME_SIZE];
	LandruHandle data;
	uint32_t file_size;
	uint32_t read_offset;
	int loaded;
} StreamChain;

typedef struct {
#ifdef LANDRU_GAME_XW
	StreamChain* chain_list;
	size_t chain_count;
	size_t chain_capacity;
	uint32_t read_limit;
#else
	StreamChain chain_list[LANDRU_STREAM_MAX_CHAINS];
	int cur_chain;
	int dest_chain;
	int use_chain;
	int chain_count;
#endif
	int stream_active;
} StreamEngine;

static StreamEngine engines[LANDRU_STREAM_CHANNEL_COUNT] = { 0 };

/* --- Lifecycle --- */

int16_t xstream_Init_Stream_Engine(int channel, int32_t buff_size, int32_t prefetch_amount) {
	if (channel < 0 || channel >= LANDRU_STREAM_CHANNEL_COUNT)
		return 0;
	StreamEngine* engine = &engines[channel];
#ifdef LANDRU_GAME_XW
	if (buff_size < 0 || prefetch_amount < 0)
		return 0;
	xstream_Exit_Stream_Engine(channel);
	engine->read_limit = (uint32_t)(buff_size / LANDRU_STREAM_BLOCK_SIZE) * LANDRU_STREAM_BLOCK_SIZE;
#else
	(void)buff_size;
	(void)prefetch_amount;
	xstream_Exit_Stream_Engine(channel);
	memset(engine->chain_list, 0, sizeof(engine->chain_list));
	engine->cur_chain = 0;
	engine->dest_chain = 0;
	engine->use_chain = 0;
	engine->chain_count = 0;
#endif
	engine->stream_active = 1;
	return 1;
}

int16_t xstream_Exit_Stream_Engine(int channel) {
	if (channel < 0 || channel >= LANDRU_STREAM_CHANNEL_COUNT)
		return 0;
	StreamEngine* engine = &engines[channel];
#ifdef LANDRU_GAME_XW
	for (size_t i = 0; i < engine->chain_count; i++)
		xmemhdl_Free_Handle(engine->chain_list[i].data);
	free(engine->chain_list);
	memset(engine, 0, sizeof(*engine));
#else
	for (int i = 0; i < LANDRU_STREAM_MAX_CHAINS; i++) {
		xmemhdl_Free_Handle(engine->chain_list[i].data);
		engine->chain_list[i].data = LANDRU_NULL_HANDLE;
	}
	memset(engine->chain_list, 0, sizeof(engine->chain_list));
	engine->stream_active = 0;
#endif
	return 1;
}

/* --- Chain management --- */

int16_t xstream_Chain_Stream_File(int channel, const char* filename) {
	if (channel < 0 || channel >= LANDRU_STREAM_CHANNEL_COUNT)
		return 0;
	StreamEngine* engine = &engines[channel];
#ifdef LANDRU_GAME_XW
	if (!engine->stream_active || !filename || !*filename)
		return 0;
	size_t length = strlen(filename);
	if (length >= LANDRU_STREAM_FILENAME_SIZE)
		return 0;
	if (engine->chain_count == engine->chain_capacity) {
		if (engine->chain_capacity > SIZE_MAX / sizeof(*engine->chain_list) / 2)
			return 0;
		size_t capacity = engine->chain_capacity ? engine->chain_capacity * 2 : 1;
		StreamChain* chains = realloc(engine->chain_list, capacity * sizeof(*chains));
		if (!chains)
			return 0;
		engine->chain_list = chains;
		engine->chain_capacity = capacity;
	}
	StreamChain* chain = &engine->chain_list[engine->chain_count];
	memset(chain, 0, sizeof(*chain));
	memcpy(chain->filename, filename, length + 1);
	engine->chain_count++;
#else
	if (!engine->stream_active || engine->chain_count >= LANDRU_STREAM_MAX_CHAINS)
		return 0;

	strncpy(engine->chain_list[engine->dest_chain].filename, filename, LANDRU_STREAM_FILENAME_SIZE - 1);
	engine->chain_list[engine->dest_chain].filename[LANDRU_STREAM_FILENAME_SIZE - 1] = '\0';
	engine->chain_list[engine->dest_chain].data = LANDRU_NULL_HANDLE;
	engine->chain_list[engine->dest_chain].file_size = 0;
	engine->chain_list[engine->dest_chain].read_offset = 0;
	engine->chain_list[engine->dest_chain].loaded = 0;

	engine->chain_count++;
	if (++engine->dest_chain >= LANDRU_STREAM_MAX_CHAINS)
		engine->dest_chain = 0;
#endif
	return 1;
}

int16_t xstream_Unchain_Current_Stream_File(int channel) {
	if (channel < 0 || channel >= LANDRU_STREAM_CHANNEL_COUNT)
		return 0;
	StreamEngine* engine = &engines[channel];
#ifdef LANDRU_GAME_XW
	if (!engine->stream_active)
		return 0;
	if (engine->chain_count) {
		xmemhdl_Free_Handle(engine->chain_list[0].data);
		engine->chain_count--;
		if (engine->chain_count)
			memmove(engine->chain_list, &engine->chain_list[1],
					engine->chain_count * sizeof(*engine->chain_list));
		memset(&engine->chain_list[engine->chain_count], 0, sizeof(*engine->chain_list));
	}
#else
	if (!engine->stream_active || engine->chain_count == 0)
		return 0;
	if (engine->use_chain == engine->cur_chain) {
		xmemhdl_Free_Handle(engine->chain_list[engine->cur_chain].data);
		engine->chain_list[engine->cur_chain].data = LANDRU_NULL_HANDLE;
		engine->chain_list[engine->cur_chain].loaded = 0;
		engine->chain_count--;
		if (++engine->cur_chain >= LANDRU_STREAM_MAX_CHAINS)
			engine->cur_chain = 0;
	}
	if (++engine->use_chain >= LANDRU_STREAM_MAX_CHAINS)
		engine->use_chain = 0;
#endif
	return 1;
}

#ifdef LANDRU_GAME_XW
int16_t xstream_Rotate_Current_Stream_File(int channel) {
	if (channel < 0 || channel >= LANDRU_STREAM_CHANNEL_COUNT)
		return 0;
	StreamEngine* engine = &engines[channel];
	if (!engine->stream_active || !engine->chain_count)
		return 0;
	char current_filename[LANDRU_STREAM_FILENAME_SIZE];
	char next_filename[LANDRU_STREAM_FILENAME_SIZE];
	memcpy(current_filename, engine->chain_list[0].filename, sizeof(current_filename));
	size_t next = engine->chain_count > 1 ? 1 : 0;
	memcpy(next_filename, engine->chain_list[next].filename, sizeof(next_filename));
	xstream_Unchain_Current_Stream_File(channel);
	xstream_Chain_Stream_File(channel, current_filename);
	xstream_Use_Stream_File(channel, next_filename);
	return 1;
}
#endif

/* --- Load and use a chained file --- */

static int load_chain_file(StreamEngine* engine, int idx) {
	if (engine->chain_list[idx].loaded)
		return 1;

	LandruFile* f = xfile_Open_File(LANDRU_FILE_ROOT_ASSET, engine->chain_list[idx].filename, "rb");
	if (!f)
		return 0;

#ifdef LANDRU_GAME_XW
	if (xfile_Seek_File(f, 0, LANDRU_SEEK_END) != 0) {
		xfile_Close_File(f);
		return 0;
	}
#else
	xfile_Seek_File(f, 0, LANDRU_SEEK_END);
#endif
	long size = xfile_Tell_File(f);
#ifdef LANDRU_GAME_XW
	if (xfile_Seek_File(f, 0, LANDRU_SEEK_SET) != 0) {
		xfile_Close_File(f);
		return 0;
	}
	if (size == 0) {
		xfile_Close_File(f);
		engine->chain_list[idx].loaded = 1;
		return 1;
	}
#else
	xfile_Seek_File(f, 0, LANDRU_SEEK_SET);
#endif

	if (size <= 0 || (uint64_t)size > UINT32_MAX) {
		xfile_Close_File(f);
		return 0;
	}
	engine->chain_list[idx].data = xres_Resource_Data_To_Handle(f, (uint32_t)size, LANDRU_MEMORY_DEFAULT);
	xfile_Close_File(f);
	if (!engine->chain_list[idx].data)
		return 0;

	engine->chain_list[idx].file_size = size;
	engine->chain_list[idx].read_offset = 0;
	engine->chain_list[idx].loaded = 1;
	return 1;
}

int16_t xstream_Use_Stream_File(int channel, const char* filename) {
	if (channel < 0 || channel >= LANDRU_STREAM_CHANNEL_COUNT)
		return 0;
	StreamEngine* engine = &engines[channel];
#ifdef LANDRU_GAME_XW
	if (!engine->stream_active || !filename || !*filename)
		return 0;
	size_t length = strlen(filename);
	if (length >= LANDRU_STREAM_FILENAME_SIZE)
		return 0;
	size_t selected = 0;
	for (; selected < engine->chain_count; selected++) {
		if (strncmp(filename, engine->chain_list[selected].filename, length) == 0)
			break;
	}
	if (selected == engine->chain_count)
		return 0;
	for (size_t i = 0; i < selected; i++)
		xstream_Unchain_Current_Stream_File(channel);
	if (!load_chain_file(engine, 0))
		return 0;
	engine->chain_list[0].read_offset = 0;
#else
	if (!engine->stream_active)
		return 0;
	int i = engine->cur_chain;
	int found = 0;
	for (int count = 0; count < engine->chain_count; count++) {
		i = (engine->cur_chain + count) % LANDRU_STREAM_MAX_CHAINS;
		if (strncmp(filename, engine->chain_list[i].filename, LANDRU_STREAM_FILENAME_SIZE) == 0) {
			found = 1;
			break;
		}
	}
	if (!found)
		return 0;

	engine->use_chain = i;

	/* Load if not already in memory */
	if (!engine->chain_list[i].loaded) {
		if (!load_chain_file(engine, i))
			return 0;
	}

	engine->chain_list[engine->use_chain].read_offset = 0;
#endif
	return 1;
}

/* --- Read from the loaded buffer --- */

int32_t xstream_Read_From_Stream_Buffer(int channel, LandruHandle destination, uint32_t offset, uint32_t size,
										int16_t blocking) {
	if (channel < 0 || channel >= LANDRU_STREAM_CHANNEL_COUNT)
		return 0;
	StreamEngine* engine = &engines[channel];
	(void)blocking; /* No async — data is always available */

#ifdef LANDRU_GAME_XW
	if (!engine->stream_active || !engine->chain_count || size > engine->read_limit)
		return 0;
	StreamChain* chain = &engine->chain_list[0];
#else
	StreamChain* chain = &engine->chain_list[engine->use_chain];
#endif
	if (!engine->stream_active || !chain->loaded || !chain->data)
		return 0;

	uint32_t available = chain->file_size - chain->read_offset;
	if (size > available)
		size = available;

	uint8_t* dest = xmemhdl_Lock_Handle(destination);
	const uint8_t* source = xmemhdl_Lock_Handle(chain->data);
	uint32_t capacity = xmemhdl_Get_Handle_Size(destination);
	if (!dest || !source || offset > capacity || size > capacity - offset || size > INT32_MAX)
		return 0;
	memcpy(dest + offset, source + chain->read_offset, size);
	chain->read_offset += size;
	return size;
}

/* --- No-ops for timing (irrelevant with synchronous I/O) --- */

int16_t xstream_Get_Stream_Flag(void) { return 0; /* STREAM_IDLE */ }

void xstream_Adjust_For_Penalty(uint32_t size) { (void)size; }

#ifdef LANDRU_GAME_XW
void xstream_ConfigureChannel(int channel, uint8_t bypassPenalty, char driveLetter) {
	(void)channel;
	(void)bypassPenalty;
	(void)driveLetter;
}

void xstream_Set_Stream_Tick_Counts(char driveLetter, const char* benchmarkFilename) {
	(void)driveLetter;
	(void)benchmarkFilename;
}
#else
void xstream_Set_Stream_Tick_Counts(void) { /* CD timing calibration — not needed */ }
#endif
