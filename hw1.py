import os 


def product_of_five():
	file = open("num.txt", "r")

	number = file.read()

	max = 0

	for index, value in enumerate(number):
		if index < 996:
			current = int(value) * int(number[index + 1]) * int(number[index + 2]) * int(number[index + 3]) * int(number[index + 4])
			if current > max:
				max = current

	
	print("The max is: " + str(max))

	file.close()

def depth_first():
	print("depth first")

	root = "/~/"

	for current_directory, sub_directory_list, list_of_files in os.walk(root):
		print("Directory name: " + current_directory)
		print("Sub dirs: " + sub_directory_list)

def alphabet(x):
	alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

	for index, value in enumerate(alpha):
		if x == value:
			return index + 1

	return 0


def file():
	counter = 0
	word = 0
	
	file = open("names.txt", "r")

	list = file.read()

	list = list.split(',')

	list = sorted(list)

	for index, value in enumerate(list):
		for index2, value in enumerate(list[index]):
			word = word + alphabet(list[index][index2])
		word = word * (index + 1)
		counter = counter + word
		word = 0
		
	
	print("The sum is: " + str(counter))
	file.close()


def main():
	option = int(input("What program do you want (1 - 3): "))

	if option == 1:
		product_of_five()

	elif option == 2:
		depth_first()

	elif option == 3:
		file()


if __name__ == "__main__":
	main()
