# 字体文件

`departure-mono-11px-square.ttf` 为 [Departure Mono](https://github.com/rektdeckard/departure-mono)
的 11px square 版本，作者 Helena Zhang & Tobias Fried，以 MIT 许可证发布（见同目录 `LICENSE`）。

OLED 界面（`daikin-bry88ab151k-pm2005-oled.yaml` / `daikin-bry88ab151k-pm2105-oled.yaml`）
的字形由该字体裁剪生成（`glyphs` 仅保留实际用到的字符），因此编译时必须保留此文件。
如需更换字体，请同步修改 YAML 中 `font:` 各条目的 `file:` 路径。
