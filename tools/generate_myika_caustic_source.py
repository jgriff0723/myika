from __future__ import annotations

import math
from pathlib import Path

from PIL import Image


FRAME_COLUMNS = 4
FRAME_ROWS = 4
FRAME_COUNT = FRAME_COLUMNS * FRAME_ROWS
FRAME_SIZE = 128


def ridge(value: float, sharpness: float) -> float:
	value = 1.0 - min(1.0, abs(value))
	return max(0.0, value) ** sharpness


def sample_caustic(u: float, v: float, phase: float) -> float:
	warp_x = math.sin((u * 7.0 + phase * 1.7) * math.tau) * 0.18
	warp_y = math.cos((v * 6.0 - phase * 1.3) * math.tau) * 0.14

	p1 = math.sin((u * 5.2 + v * 1.8 + warp_y + phase * 0.85) * math.tau)
	p2 = math.cos((u * -2.6 + v * 6.1 + warp_x - phase * 1.1) * math.tau)
	p3 = math.sin(((u + v) * 4.4 + warp_x - warp_y + phase * 1.45) * math.tau)

	line_a = ridge(p1 + p2 * 0.55, 6.0)
	line_b = ridge(p2 - p3 * 0.45, 7.0)
	line_c = ridge(p3 + p1 * 0.35, 8.0)

	value = line_a * 0.55 + line_b * 0.75 + line_c * 0.65
	return max(0.0, min(1.0, value))


def generate_flipbook() -> Image.Image:
	width = FRAME_COLUMNS * FRAME_SIZE
	height = FRAME_ROWS * FRAME_SIZE
	image = Image.new("RGBA", (width, height), (0, 0, 0, 255))
	pixels = image.load()

	for frame_index in range(FRAME_COUNT):
		frame_phase = frame_index / float(FRAME_COUNT)
		tile_x = frame_index % FRAME_COLUMNS
		tile_y = frame_index // FRAME_COLUMNS

		for py in range(FRAME_SIZE):
			for px in range(FRAME_SIZE):
				u = (px + 0.5) / FRAME_SIZE
				v = (py + 0.5) / FRAME_SIZE
				value = sample_caustic(u, v, frame_phase)
				tint = int(255.0 * value)
				x = tile_x * FRAME_SIZE + px
				y = tile_y * FRAME_SIZE + py
				pixels[x, y] = (tint, tint, tint, 255)

	return image


def main() -> None:
	output_dir = Path(__file__).resolve().parent / "Generated" / "MyikaWater"
	output_dir.mkdir(parents=True, exist_ok=True)
	output_path = output_dir / "T_MyikaCaustic_Flipbook.png"
	generate_flipbook().save(output_path)
	print(output_path)


if __name__ == "__main__":
	main()
