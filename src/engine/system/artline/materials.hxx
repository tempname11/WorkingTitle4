#pragma once
#include "public.hxx"

#define ASSET_PATH(str) lib::cstr::from_static("assets/" str)
#define AMBIENTCG_JPG(str, type) ASSET_PATH("ambientcg/" str "-JPG/" str "_" type ".jpg")
#define AMBIENTCG_PNG(str, type) ASSET_PATH("ambientcg/" str "-PNG/" str "_" type ".png")
#define AMBIENTCG_JPG_TRIPLE(str) { \
  .file_path_albedo = AMBIENTCG_JPG(str, "Color"), \
  .file_path_normal = AMBIENTCG_JPG(str, "NormalGL"), \
  .file_path_romeao = AMBIENTCG_JPG(str, "Roughness"), \
}
#define AMBIENTCG_PNG_TRIPLE(str) { \
  .file_path_albedo = AMBIENTCG_PNG(str, "Color"), \
  .file_path_normal = AMBIENTCG_PNG(str, "NormalGL"), \
  .file_path_romeao = AMBIENTCG_PNG(str, "Roughness"), \
}

namespace engine::system::artline::materials {

PieceMaterial placeholder = {
  .file_path_albedo = ASSET_PATH("texture-1px/albedo.png"),
  .file_path_normal = ASSET_PATH("texture-1px/normal.png"),
  .file_path_romeao = ASSET_PATH("texture-1px/romeao.png"),
};

namespace ambientcg {

PieceMaterial Bricks059 = AMBIENTCG_JPG_TRIPLE("Bricks059_8K");
PieceMaterial Concrete034 = AMBIENTCG_JPG_TRIPLE("Concrete034_8K");
PieceMaterial Marble020 = AMBIENTCG_JPG_TRIPLE("Marble020_8K");
PieceMaterial Metal035 = AMBIENTCG_JPG_TRIPLE("Metal035_8K");
PieceMaterial PavingStones107 = AMBIENTCG_JPG_TRIPLE("PavingStones107_8K");
PieceMaterial Terrazzo009 = AMBIENTCG_JPG_TRIPLE("Terrazzo009_8K");

} // namespace

} // namespace