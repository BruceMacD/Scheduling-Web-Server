Group Project: Scheduling Web Server 

Project authors:
	Bruce MacDonald		B00580971
	Johna Latouf		B00698246
	Brandon Bremner		B00675619
	Ryan Stevens		B00695460
	
	Contains code given to the authors, designated for this project.
	
Course:
	Operating Systems (CSCI 3120)
	Summer 2017

-----------------------------
HOW TO OPERATE

	Compile project:
		Simply navigate to correct folder and run "make"
		
	Start web server:
		./sws 38080 <Algorithm> <Threads>
		Where			
			<Algorithm> is replaced with one of the following acronyms:
				SJF					(Shortest Job First)
				RR					(Round Robin)
				MLFB				(Multi-Level Feedback)
			
			<Threads> is replaced with a number from 1 to 100
			
	Run test cases:
		Run whichever of these is appropriate for your computer. Both do the same thing
			./hydra.py < test.#.in > Test.#.$.out
			python ./hydra.py < test.#.in > Test.#.$.out
				Where '#' is replaced with a number 0 to 9.
				And where '$' is replaced with 'M', 'R', or 'S', depending on which scheduler algorithm is being used
					M = Multi-Level = MLFB
					R = Round Robine = RR
					S = Shortest Job First = SJF
				This runs hydra.py, taking your test case in as a parameter, and prints the results out to a file.