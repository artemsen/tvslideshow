#!/bin/bash -eu

# Photo converter:
# - resize to TV display size
# - add background to get 16:9 aspect
# - add title (directory name)
# - strip EXIF data

TARGET_SIZE="4096x2160"
FONT_NAME="Play-Regular"
FONT_SIZE=80
FONT_COLOR="#cccccc"

# convert image file to TV format
convert_file() {
  local src_file=$1
  local dst_file=$2
  local dir_name=${src_file%/*}
  dir_name=${dir_name##*/}

  magick "${src_file}" \
    -auto-orient -resize "${TARGET_SIZE%x*}" -gravity Center \
    -crop "${TARGET_SIZE}+0+0" -blur 0x40 \
    -gravity Center "${src_file}" -auto-orient -resize "${TARGET_SIZE}" -composite \
    \( -background none -pointsize ${FONT_SIZE} -gravity SouthEast \
       -fill "${FONT_COLOR}" -font "${FONT_NAME}" label:"${dir_name}" \
      \( +clone -background black -shadow 80x4+0+0 \) \
      -geometry +10+10 +swap -background none -layers merge +repage \
    \) -composite \
    -quality 80% -strip \
    "${dst_file}"
}

# convert photos for TV
convert_dir() {
  local src_dir=$1
  src_dir=$(realpath "${src_dir}")
  local dst_dir=$2

  local find_cmd=(find "${src_dir}" -mindepth 1 -maxdepth 1 -name '*.jpg')

  local total
  total=$("${find_cmd[@]}" | wc -l)
  local current=1

  mkdir -p "${dst_dir}"

  local src
  "${find_cmd[@]}" -print0 | sort -z | while read -rd $'\0' src; do
    local name="${src##*/}"
    echo -n "Convert ${current} of ${total}: ${name}... "
    convert_file "${src}" "${dst_dir}/${name}"
    echo "OK"
    current=$((current + 1))
  done
}

# entry point
if [[ $# -lt 2 || $1 == --help ]]; then
  echo "Use: $0 SRCDIR DSTDIR"
  exit 1
fi
convert_dir "$1" "$2"
