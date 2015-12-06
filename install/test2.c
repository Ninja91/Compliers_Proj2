main()
{
        float   x, y;

        printf("Please input 1st value: ");
        scanf("%f", &x);

	while (1)
	{
           if (x > 0.0) y = 4.0;
	   else break;
           x = x - y;
	}

        printf("Values AFTER %f, %f\n", x, y);
        return 0;
}
