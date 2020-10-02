#include <bits/stdc++.h>
using namespace std;
void print(vector<string> names){
	printf("printing ........\n");
	for(int i=0;i<names.size();i++)
		cout<<names[i]<<endl;
	printf("\n");
}

bool mycomp(string a, string b){
	return a<b;
}

vector<string> alphabaticallySort(vector<string> a){
	int n=a.size();
	sort(a.begin(),a.end(),mycomp);
	return a;
}

int main()
{   
	int n;
	printf("enter all the names");
	scanf("%d",&n);
  vector<string> names;
	string name;
	printf("enter names: \n");
	//taking input
	for(int i=0;i<n;i++){
		cin>>name;
		//insert names into the vector
		names.push_back(name); 
	}

	printf("\nbefore sorting\n");
	print(names);
	names=alphabaticallySort(names);

	printf("after alphabetical sorting\n");
	print(names);

	return 0;
}
