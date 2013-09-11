all: sum2pi_x sum2pi_x-float sum2pi_x-mixed

test: all
	./sum2pi_x
	./sum2pi_x-float
	./sum2pi_x-mixed

sum2pi_x: sum2pi_x.c
	gcc -g -O2 -o sum2pi_x sum2pi_x.c

sum2pi_x-float: sum2pi_x.c
	gcc -g -O2 -Dreal=float -o sum2pi_x-float sum2pi_x.c

sum2pi_x-mixed: sum2pi_x.c
	gcc -g -O2 -Dreal=float -Dsum_type=double -o sum2pi_x-mixed sum2pi_x.c

clean:
	rm -f sum2pi_x sum2pi_x-float sum2pi_x-mixed

