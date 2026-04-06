../cutechess/cutechess/build/cutechess-cli \
 -engine cmd=./chessmatrix_master proto=uci name=OLD \
 -engine cmd=./chessmatrix proto=uci name=NEW \
 -each tc=8+0.01 \
 -games 200 \
 -repeat \
 -concurrency 4 \
 -openings file=UHO_4060_v4.epd format=epd order=random