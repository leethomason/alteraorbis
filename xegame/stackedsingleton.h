#ifndef STACKED_SINGLETON_INCLUDED
#define STACKED_SINGLETON_INCLUDED

/* doesn't work - needs a static template member. grr. */
template< class T >
class StackedSingleton
{
public:
	StackedSingleton() : lastInstance( 0 ) {}

	static T* Instance() { return instance; }

protected:
	void PushInstance( T* t )	{ lastInstance = instance; instance = t; }
	void PopInstance( T* t )	{ instance = lastInstance; lastInstance = 0; }

private:
	static T* instance;
	T* lastInstance;
};


#endif // STACKED_SINGLETON_INCLUDED
