from __future__ import annotations

import math
from pathlib import Path

from PIL import Image


SIZE = 256


def height(u: float, v: float) -> float:
	h = 0.0
	h += math.sin((u * 2.0 + v * 1.0) * math.tau) * 0.50
	h += math.sin((u * 4.0 - v * 3.0) * math.tau + 1.7) * 0.25
	h += math.sin((u * 7.0 + v * 5.0) * math.tau + 3.1) * 0.12
	h += math.sin((u * -3.0 + v * 9.0) * math.tau - 2.4) * 0.10
	return h


def generate_normal() -> Image.Image:
	image = Image.new("RGBA", (SIZE, SIZE), (128, 128, 255, 255))
	pixels = image.load()
	step = 1.0 / SIZE
	amplitude = 0.65

	for py in range(SIZE):
		for px in range(SIZE):
			u = px / SIZE
			v = py / SIZE
			hl = height((u - step) % 1.0, v)
			hr = height((u + step) % 1.0, v)
			hd = height(u, (v - step) % 1.0)
			hu = height(u, (v + step) % 1.0)

			nx = (hl - hr) * amplitude
			ny = (hd - hu) * amplitude
			nz = 1.0
			length = math.sqrt(nx * nx + ny * ny + nz * nz)
			nx /= length
			ny /= length
			nz /= length

			r = int(round((nx * 0.5 + 0.5) * 255.0))
			g = int(round((ny * 0.5 + 0.5) * 255.0))
			b = int(round((nz * 0.5 + 0.5) * 255.0))
			pixels[px, py] = (r, g, b, 255)

	return image


def main() -> None:
	output_dir = Path(__file__).resolve().parent / "Generated" / "MyikaWater"
	output_dir.mkdir(parents=True, exist_ok=True)
	output_path = output_dir / "T_MyikaWater_Normal.png"
	generate_normal().save(output_path)
	print(output_path)


if __name__ == "__main__":
	main()
