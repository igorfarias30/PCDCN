clf();
ax=gca();
ax.data_bounds=[0,0;2, 3]; //set the data_bounds
ax.box='on'; //desenha uma caixa
a = 5  * ones(11,11); a(2:10,2:10)=34; a(6.5:7,6.5:7)=5;
b = 1 * ones(11,11); b(2:10,2:10)=1; b(6.5:7,6.5:7)=1;
L = list();
//Porto 1
S = [2 2;2 2;2 0;];
L(1) = S;

//Porto 2
S = [3 100;3 3;0 0;];
L(2) = S;

//Porto 3
S = [4 100;0 4;0 0;];
L(3) = S;

for k = 1:3
	gcf();
	for i = 2: -1:0
		for j = 1:-1:0
			if(L(k)(i+1,j+1) <> 0)
				a(6.5:7,6.5:7) = L(k)(i + 1, j + 1);
				Matplot1(a, [j, i, j + 1, i + 1]);
			else
				Matplot1(b, [j, i, j + 1, i + 1]);
			end
		end
	end
	sleep(2000);
end
clf(gcf(), 'reset')
