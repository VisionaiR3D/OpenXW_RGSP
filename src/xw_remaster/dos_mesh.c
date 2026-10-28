/* Cached palette geometry, following OpenTIE classic_mesh.c. Source vertices and
 * normals have already been decoded by X-Wing's bounded model views. */
#include "xw_remaster/dos_mesh_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_BUFFER_BYTES = 64 * 1024 * 1024 };

typedef char DosVertexLayout[sizeof(XwDosVertex) == 92 ? 1 : -1];
typedef char DosMarkLayout[sizeof(XwDosMark) == 32 ? 1 : -1];

bool XwDosMesh_Grow(void** data, uint32_t* capacity, uint32_t need, size_t stride) {
	if (need <= *capacity)
		return true;
	if (need > MAX_BUFFER_BYTES / stride)
		return false;
	uint32_t next = *capacity ? *capacity : 64;
	while (next < need && next <= MAX_BUFFER_BYTES / stride / 2)
		next *= 2;
	if (next < need)
		next = need;
	void* resized = realloc(*data, next * stride);
	if (!resized)
		return false;
	*data = resized;
	*capacity = next;
	return true;
}

static bool AddLine(XwDosMeshBuild* b, const XwRenderDosMesh* mesh, const XwRenderDosFace* face,
					unsigned first, unsigned second, unsigned width, unsigned color, unsigned flags) {
	if (first >= mesh->vertex_count || second >= mesh->vertex_count ||
		!XwDosMesh_Grow((void**)&b->vertices, &b->vertex_capacity, b->vertex_count + 4,
						sizeof *b->vertices) ||
		!XwDosMesh_Grow((void**)&b->indices, &b->index_capacity, b->index_count + 6, sizeof *b->indices))
		return false;
	static const unsigned corners[6] = { 0, 1, 2, 2, 1, 3 };
	for (unsigned i = 0; i < 6; ++i)
		b->indices[b->index_count++] = b->vertex_count + corners[i];
	for (unsigned i = 0; i < 4; ++i) {
		XwDosVertex v = { .info = { color, flags | XW_DOS_LINE, 0, 0 }, .line = { width, i } };
		for (unsigned a = 0; a < 3; ++a) {
			v.position[a] = mesh->vertices[first][a];
			v.other[a] = mesh->vertices[second][a];
			v.normal[a] = face ? face->normal[a] / 32768.0f : 0;
			v.vertex_normal[a] =
				mesh->authored_normals ? mesh->normals[i / 2 ? second : first][a] / 32768.0f : v.normal[a];
			v.other_normal[a] =
				mesh->authored_normals ? mesh->normals[i / 2 ? first : second][a] / 32768.0f : v.normal[a];
		}
		b->vertices[b->vertex_count++] = v;
	}
	return true;
}

static bool Polygon(XwDosMeshBuild* b, const XwRenderDosMesh* mesh, const XwRenderDosFace* face,
					unsigned id) {
	unsigned count = face ? face->vertex_count : mesh->vertex_count;
	if (count < 3)
		return true;
	if (!XwDosMesh_Grow((void**)&b->vertices, &b->vertex_capacity, b->vertex_count + count,
						sizeof *b->vertices))
		return false;
	/* Dropping the dominant plane axis preserves concavity and authored winding. */
	float normal[3] = { 0, 0, 1 };
	if (face)
		for (unsigned a = 0; a < 3; ++a)
			normal[a] = face->normal[a] / 32768.0f;
	unsigned drop = 0;
	for (unsigned a = 1; a < 3; ++a)
		if (fabsf(normal[a]) > fabsf(normal[drop]))
			drop = a;
	unsigned u = (drop + 1) % 3, v = (drop + 2) % 3;
	XwSnapRange marks = { 0 };
	if (face && !XwDosMesh_Marks(b, mesh, id, u, v, &marks))
		return false;
	unsigned flags = (!(mesh->format & 1) || mesh->format == 0xFF) ? XW_DOS_RAW_COLOR : 0;
	if (!face || face->two_sided)
		flags |= XW_DOS_TWO_SIDED;
	if (face && face->gouraud && mesh->authored_normals)
		flags |= XW_DOS_GOURAUD;
	uint32_t base = b->vertex_count;
	float uv[255][2];
	for (unsigned i = 0; i < count; ++i) {
		unsigned index = face ? face->indices[i] : i;
		if (index >= mesh->vertex_count)
			return false;
		XwDosVertex vertex = { .info = { face ? face->color : mesh->color, flags, marks.first,
										 marks.count } };
		for (unsigned a = 0; a < 3; ++a) {
			vertex.position[a] = mesh->vertices[index][a];
			vertex.normal[a] = normal[a];
			vertex.vertex_normal[a] = mesh->authored_normals ? mesh->normals[index][a] / 32768.0f : normal[a];
		}
		vertex.uv[0] = uv[i][0] = vertex.position[u];
		vertex.uv[1] = uv[i][1] = vertex.position[v];
		b->vertices[b->vertex_count++] = vertex;
	}
	return XwDosMesh_Triangulate(uv, count, base, b);
}

static bool Lod(XwDosMeshBuild* b, const XwRenderDosMesh* mesh, XwDosMeshLod* out) {
	uint32_t first_vertex = b->vertex_count;
	out->triangles.first = b->index_count;
	if (mesh->format == 0xFF) {
		if (!Polygon(b, mesh, NULL, 0))
			return false;
	} else
		for (unsigned i = 0; i < mesh->face_count; ++i)
			if (mesh->faces[i].vertex_count != 2 && !Polygon(b, mesh, &mesh->faces[i], i))
				return false;
	out->triangles.count = b->index_count - out->triangles.first;
	out->lines.first = b->index_count;
	for (unsigned i = 0; i < mesh->face_count; ++i) {
		const XwRenderDosFace* face = &mesh->faces[i];
		if (face->vertex_count != 2)
			continue;
		unsigned flags = face->two_sided ? XW_DOS_TWO_SIDED : 0;
		if (!(mesh->format & 1))
			flags |= XW_DOS_RAW_COLOR;
		if (face->gouraud && mesh->authored_normals)
			flags |= XW_DOS_GOURAUD;
		if (!AddLine(b, mesh, face, face->indices[0], face->indices[1], face->line_width, face->color, flags))
			return false;
	}
	if (mesh->format == 0x40 || mesh->format == 0x41) {
		if (mesh->lines_offset > b->model->size ||
			5u * mesh->edge_count > b->model->size - mesh->lines_offset)
			return false;
		for (unsigned i = 0; i < mesh->edge_count; ++i) {
			const uint8_t* p = b->model->bytes + mesh->lines_offset + 5 * i;
			if (!AddLine(b, mesh, NULL, p[2], p[3], p[0] | (p[1] << 8), p[4],
						 XW_DOS_RAW_COLOR | XW_DOS_TWO_SIDED))
				return false;
		}
	}
	out->lines.count = b->index_count - out->lines.first;
	for (unsigned i = first_vertex; i < b->vertex_count; ++i) {
		const XwDosVertex* vertex = &b->vertices[i];
		for (unsigned endpoint = 0; endpoint < ((unsigned)vertex->info[1] & XW_DOS_LINE ? 2u : 1u);
			 ++endpoint) {
			const float* point = endpoint ? vertex->other : vertex->position;
			float radius = sqrtf(point[0] * point[0] + point[1] * point[1] + point[2] * point[2]);
			out->radius = fmaxf(out->radius, radius + 2 * vertex->line[0]);
		}
	}
	return true;
}

static bool Upload(AeronCommandBuffer* cmd, AeronBuffer** out, const void* data, size_t size,
				   unsigned usage) {
	static const uint8_t empty[32] = { 0 };
	if (!size) {
		size = sizeof empty;
		data = empty;
	}
	*out = Aeron_CreateBuffer(&(AeronBufferDesc) {
		.usage = usage, .size = (uint32_t)size, .memory_usage = AERON_MEMORY_USAGE_GPU_ONLY });
	return *out && Aeron_UploadBufferDataCmd(cmd, *out, 0, data, (uint32_t)size);
}

/* Optional HD exterior for the isolated RGSP test build. Local, versioned assets. */
static bool HdRead(FILE* f, void* p, size_t size) { return fread(p, 1, size, f) == size; }
#include "xw_remaster/hd_graphics.inc"
static bool HdLoad(AeronCommandBuffer* cmd, XwDosMeshBuild* b, XwDosMesh* out, unsigned model_id) {
 if (!XwHdGraphics_Active()) return false;
 char path[80];
 if(model_id==1)snprintf(path,sizeof path,"hd/xwing.hdmesh");else snprintf(path,sizeof path,"hd/model-%u.hdmesh",model_id);
 FILE* f=fopen(path,"rb");
 if(!f)return false;
 char magic[8];uint32_t header[3],ranges[64][3];
 bool ok=HdRead(f,magic,8)&&!memcmp(magic,"XWHD0001",8)&&HdRead(f,header,sizeof header);
 ok=ok&&header[0]>0&&header[0]<=120000&&header[1]>0&&header[1]<=360000&&header[1]%3==0&&header[2]>0&&header[2]<=64&&header[2]==b->model->component_count;
 XwDosVertex* v=NULL;uint32_t* idx=NULL;
 if(ok){v=malloc(header[0]*sizeof *v);idx=malloc(header[1]*sizeof *idx);ok=v&&idx;}
 if(ok)ok=HdRead(f,ranges,header[2]*sizeof ranges[0])&&HdRead(f,v,header[0]*sizeof *v)&&HdRead(f,idx,header[1]*sizeof *idx)&&fgetc(f)==EOF;
 fclose(f);
 for(unsigned i=0;ok&&i<header[0];++i){
  const float* fields=(const float*)&v[i];
  for(unsigned j=0;j<23;++j)if(!isfinite(fields[j]))ok=false;
  for(unsigned j=0;j<3;++j)if(fabsf(v[i].position[j])>32768)ok=false;
  if(v[i].info[1]==82 && (v[i].info[3]<1 || v[i].info[3]>8 || v[i].info[2]<0 || v[i].info[2]>=v[i].info[3]*v[i].info[3]))ok=false;
  if(v[i].info[1]!=18&&v[i].info[1]!=50&&v[i].info[1]!=82&&v[i].info[1]!=146&&v[i].info[1]!=210)ok=false;
 }
 for(unsigned i=0;ok&&i<header[1];++i)if(idx[i]>=header[0])ok=false;
 for(unsigned i=0;ok&&i<header[2];++i)if(ranges[i][0]!=b->model->components[i].first||ranges[i][0]>=out->lod_count||ranges[i][1]>header[1]||ranges[i][2]>header[1]-ranges[i][1]||ranges[i][2]%3)ok=false;
 AeronTexture* texture=NULL;
 if(ok){
  if(model_id==1)snprintf(path,sizeof path,"hd/xwing.hdtex");else snprintf(path,sizeof path,"hd/model-%u.hdtex",model_id);
  f=fopen(path,"rb");uint32_t levels=0,wh[2]={0},width=0,height=0;
  ok=f&&HdRead(f,magic,8)&&!memcmp(magic,"XWHT0001",8)&&HdRead(f,&levels,4)&&levels>0&&levels<=12&&HdRead(f,wh,8);
  width=wh[0];height=wh[1];
  ok=ok&&width>0&&height>0&&width<=2048&&height<=2048&&!(width&(width-1))&&!(height&(height-1));
  unsigned expected=1;for(unsigned size=width>height?width:height;size>1;size>>=1)++expected;
  ok=ok&&levels==expected;
  if(ok)texture=Aeron_CreateTexture(&(AeronTextureDesc){.width=width,.height=height,.mip_count=levels,.format=AERON_TEXTURE_FORMAT_RGBA8_SRGB,.usage=AERON_TEXTURE_USAGE_SAMPLED|AERON_TEXTURE_USAGE_TRANSFER_DST});
  ok=ok&&texture;
  for(unsigned l=0;ok&&l<levels;++l){
   unsigned w=width>>l,h=height>>l;if(!w)w=1;if(!h)h=1;
   ok=(!l||HdRead(f,wh,8))&&wh[0]==w&&wh[1]==h;
   size_t size=w*h*4;void* pixels=ok?malloc(size):NULL;ok=ok&&pixels;
   if(ok)ok=HdRead(f,pixels,size)&&Aeron_UploadTextureDataCmd(cmd,&(AeronTextureUploadDesc){.texture=texture,.mip_level=l,.width=w,.height=h,.raw_data=pixels,.raw_size=size});
   free(pixels);
  }
  if(f){if(ok)ok=fgetc(f)==EOF;fclose(f);}
 }
 if(ok)ok=XwDosMesh_Grow((void**)&b->vertices,&b->vertex_capacity,b->vertex_count+header[0],sizeof *v)&&XwDosMesh_Grow((void**)&b->indices,&b->index_capacity,b->index_count+header[1],sizeof *idx);
 if(ok){
  unsigned base=b->vertex_count,first=b->index_count;
  memcpy(b->vertices+base,v,header[0]*sizeof *v);
  for(unsigned i=0;i<header[1];++i)b->indices[first+i]=base+idx[i];
  b->vertex_count+=header[0];b->index_count+=header[1];
  for(unsigned i=0;i<header[2];++i){XwDosMeshLod* lod=&out->lods[ranges[i][0]];lod->triangles=(XwSnapRange){first+ranges[i][1],ranges[i][2]};lod->lines.count=0;lod->hd=true;
   for(unsigned j=0;j<ranges[i][2];++j){float* p=v[idx[ranges[i][1]+j]].position;lod->radius=fmaxf(lod->radius,sqrtf(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]));}
  }
  out->hd_texture=texture;fprintf(stderr,"HD model %u loaded: %u vertices, %u triangles\n",model_id,header[0],header[1]/3);
 }else{Aeron_DestroyTexture(texture);fprintf(stderr,"HD model %u unavailable/invalid; using original geometry\n",model_id);}
 free(v);free(idx);return ok;
}


bool XwDosMesh_Build(AeronCommandBuffer* cmd, const XwRenderSource* source, XwDosMesh* out, char* error,
					 size_t capacity) {
	if (!cmd || !source || source->kind != XW_SOURCE_DOS_MODEL || source->size != sizeof(XwRenderDosModel))
		return false;
	XwDosMeshBuild b = { .model = source->data, .dos93 = source->flight_version == 93 };
	if (b.model->image_count)
		return true;
	bool ok = b.model->lod_count && b.model->lods && b.model->bytes;
	out->lod_count = b.model->lod_count;
	out->lods = calloc(out->lod_count, sizeof *out->lods);
	ok = ok && out->lods;
	for (unsigned i = 0; ok && i < out->lod_count; ++i)
		ok = Lod(&b, &b.model->lods[i], &out->lods[i]);
	if(ok && source->flight_version == 94 && source->path) {
        const char* suffix=strrchr(source->path,'/');
        unsigned model_id=0;char trailing=0;
        if(suffix && sscanf(suffix,"/model-%u%c",&model_id,&trailing)==1 && model_id>=1 && model_id<=118) HdLoad(cmd,&b,out,model_id);
    }
    size_t vb = b.vertex_count * sizeof *b.vertices, ib = b.index_count * sizeof *b.indices;
	size_t mb = b.mark_count * sizeof *b.marks, mv = b.mark_vertex_count * sizeof *b.mark_vertices;
	ok = ok && vb + ib + mb + mv <= MAX_BUFFER_BYTES;
	if (ok)
		ok = Upload(cmd, &out->vertices, b.vertices, vb, AERON_BUFFER_USAGE_VERTEX) &&
			 Upload(cmd, &out->indices, b.indices, ib, AERON_BUFFER_USAGE_INDEX) &&
			 Upload(cmd, &out->marks, b.marks, mb, AERON_BUFFER_USAGE_STORAGE) &&
			 Upload(cmd, &out->mark_vertices, b.mark_vertices, mv, AERON_BUFFER_USAGE_STORAGE);
	free(b.vertices);
	free(b.indices);
	free(b.marks);
	free(b.mark_vertices);
	if (!ok) {
		XwDosMesh_Destroy(out);
		if (error && capacity)
			snprintf(error, capacity, "DOS mesh conversion/upload failed: %s", source->path);
	}
	return ok;
}

void XwDosMesh_Destroy(XwDosMesh* mesh) {
	Aeron_DestroyTexture(mesh->hd_texture);
	Aeron_DestroyBuffer(mesh->vertices);
	Aeron_DestroyBuffer(mesh->indices);
	Aeron_DestroyBuffer(mesh->marks);
	Aeron_DestroyBuffer(mesh->mark_vertices);
	free(mesh->lods);
	memset(mesh, 0, sizeof *mesh);
}
