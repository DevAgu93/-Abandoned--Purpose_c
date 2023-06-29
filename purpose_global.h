//
// __Reserved space
//
#define GAME_TILESIZE (12)
#define DATA_FOLDER(path) "data/" ##path
#define DATA_PATH "data/"
#define ASSET_ID_INVALID '0000'

typedef struct{
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 v3;
}vertices;

typedef struct uvs{
	union{
		vec2 u[4];
		struct{
			vec2 uv0;
			vec2 uv1;
			vec2 uv2;
			vec2 uv3;
		};
	};
}uvs;
