#ifndef	_SYS_CDEFS_H_
#define	_SYS_CDEFS_H_

#if defined(__STDC__) || defined(__cplusplus)

#define	__P(protos)	protos
#define	__CONCAT(x,y)	x ## y
#define	__STRING(x)	#x

#define	__const		const
#define	__signed	signed
#define	__volatile	volatile

#endif

#endif