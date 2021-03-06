C     Square matrix exponent
C
C     Copyright (C), Sergei Nikolaev, 1992-2006, http://cvmlib.com
C
C     Input/Output parameters:
C
C     M   - size of matrix (int)(input)
C     A   - matrix (complex)(input)
C     LDA - leading dimesion of A (int)(input)
C     EA  - exponent of A (complex)(output)
C     LDE - leading dimesion of EA (int)(input)
C     TOL - tolerance (real)(input)
C     R   - working array of size NR (real)(input). IT MUST BE FILLED WITH ZEROES
C     IR  - working array of size NI (int)(input)
C     NR  - working array R size (int)(input).  IT MUST BE CALCULATED BY 'DMEXPC'
C     NI  - working array IR size (int)(input). IT MUST BE CALCULATED BY 'DMEXPC'
C     NQ  - row length (int)(input).            IT MUST BE CALCULATED BY 'DMEXPC'
C     J   - A measure (int)(input).             IT MUST BE CALCULATED BY 'DMEXPC'
C     ISSYMM - whether matrix A is symmetric (int)(input). 1 - true, 0 - false
C     WORK   - working array of size LWORK (real)(input)
C     LWORK  - length of working array WORK (int)(input) - usually 64 * M

      SUBROUTINE DMEXP (M, A, LDA, EA, LDE, R, IR, NR, NI, NQ, J,
     1                  ISSYMM, WORK, LWORK)   ! being referenced in 
                                               ! symmetric case only
CDEC$ IF DEFINED (FTN_EXPORTS)
CDEC$     ATTRIBUTES DLLEXPORT::DMEXP
CDEC$ ENDIF
      INTEGER*4   M, LDA, LDE
      REAL*8      A(LDA*M), EA(LDE*M)
      INTEGER*4   NR, NI, NQ, J, LWORK
      LOGICAL     ISSYMM
      REAL*8      R(NR), WORK(LWORK)
      INTEGER*4   IR(NI)

      INTEGER*4   I, IQ, NQC, NB, INFO
      INTEGER*4   MM, MM1, MM12, MM13, MM14, MNB, MW
      REAL*8      ZERO /0.D0/, ONE /1.D0/, TWO /2.D0/
      REAL*8      TJ, C
      CHARACTER*1 TRANS /'N'/
      CHARACTER*1 UPLO /'U'/
      LOGICAL     EVEN
      INTEGER*4   FLOOR, CEILING

      IF (M .LE. 0) RETURN
      IF (M .EQ. 1) THEN
         EA(1) = DEXP (A(1))
         RETURN
      END IF

      TJ   = 1.D0
      C    = 0.5D0

      MM   = M * M
      MM1  = MM   + 1
      MM12 = MM1  + MM
      MM13 = MM12 + MM
      MM14 = MM13 + MM

C      R = ZERO

      NQC = FLOOR (DFLOAT (NQ) / TWO) + 1
      MNB = MM14 + 2 * NQC
      NR  = FLOOR (DFLOAT (NQC - 1) /
     1             DFLOAT (CEILING (DSQRT (DFLOAT (NQC - 1)))))
      NB  = (2 * NR + 2) * MM
      MW  = MNB + NB


      EVEN = .TRUE.
      I    = 1
      R(MM14)       = ONE
      R(MM14 + NQC) = C
      DO 40 IQ = 2, NQ
          C = C * DFLOAT (NQ - IQ + 1) / DFLOAT ((2 * NQ - IQ + 1) * IQ)

          IF (EVEN) THEN
              R(MM14 + I) = C
          ELSE
              R(MM14 + NQC + I) = C
              I = I + 1
          ENDIF

          EVEN = .NOT. EVEN
40    CONTINUE

      CALL DCOPYM (M, M, A, LDA, EA, LDE)
      IF (J .GT. 0) THEN
          TJ = TWO ** DFLOAT (-J)
          CALL DSCALM (M, M, TJ, EA, LDE)
      ENDIF

      CALL DCOPYM (M, M, EA, LDE, R(MM1), M)
      CALL DGEMM (TRANS, TRANS, M, M, M, ONE, R(MM1), M,
     1                 EA, LDE, ZERO, R, M)
      CALL DSCAL (MM, ZERO, R(MM1), 1)

C     U and V matrices calculation...
      CALL DPOLY2 (M, R, NQC, R(MM14), R(MM14 + NQC), R(MM1), R(MM12),
     1                 R(MNB), R(MNB + (NR + 2) * MM))
      CALL DCOPYM (M, M, R(MM1), M, EA, LDE)
      CALL DGEMM  (TRANS, TRANS, M, M, M, TJ,
     1                 A, LDA, R(MM12), M, ONE, EA, LDE)  ! N matrix
      CALL DGEMM  (TRANS, TRANS, M, M, M, -TJ, 
     1                 A, LDA, R(MM12), M, ONE, R(MM1), M) ! D matrix
      CALL DCOPY  (MM, R(MM1), 1, R(MM13), 1)            ! copy of D
      CALL DCOPYM (M, M, EA, LDE, R(MM12), M)                ! copy of N

      IF (ISSYMM) THEN
          CALL DSYTRF (UPLO, M, R(MM1), M, IR, WORK, LWORK, INFO)
          CALL DCOPY  (MM, R(MM1), 1, R, 1)              ! copy of LD/UD
          CALL DSYTRS (UPLO, M, M, R(MM1), M, IR, EA, LDE, INFO)
          CALL DSYRFS (UPLO, M, M, R(MM13), M, R, M, IR, 
     1                 R(MM12), M, EA, LDE, R(MW), R(MW + M),
     2                 R(MW + 2*M), IR(M + 1), INFO)
      ELSE
          CALL DGETRF (M, M, R(MM1), M, IR, INFO)        ! factorize
          CALL DCOPY  (MM, R(MM1), 1, R, 1)              ! copy of LD/UD
          ! F matrix (EA) is  the solution of the equation DF = N
          CALL DGETRS (TRANS, M, M, R(MM1), M, IR, EA, LDE, INFO)
          CALL DGERFS (TRANS, M, M, R(MM13), M, R, M, IR,
     1                 R(MM12), M, EA, LDE, R(MW), R(MW + M),
     2                 R(MW + 2*M), IR(M + 1), INFO)
      ENDIF

      DO 50 I = 1, J
          CALL DCOPYM (M, M, EA, LDE, R, M)
          CALL DCOPYM (M, M, EA, LDE, R(MM1), M)
          CALL DGEMM (TRANS, TRANS, M, M, M, ONE,
     1                 R, M, R(MM1), M, ZERO, EA, LDE)     ! F = F * F
50    CONTINUE

      RETURN
      END !SUBROUTINE DMEXP


C     Input/Output parameters:
C
C     M   - size of matrix (int)(input)
C     A   - matrix (complex)(input)
C     LDA - leading dimesion of A (int)(input)
C     TOL - tolerance (real)(input)
C     NR  - working array R size (int)(output)
C     NI  - working array IR size (int)(output)
C     NQ  - row length (int)(output)
C     J   - A measure (int)(output)


      SUBROUTINE DMEXPC (M, A, LDA, TOL, NR, NI, NQ, J)
CDEC$ IF DEFINED (FTN_EXPORTS)
CDEC$     ATTRIBUTES DLLEXPORT::DMEXPC
CDEC$ ENDIF
      INTEGER*4 M, LDA
      REAL*8    A(LDA*M), TOL
      INTEGER*4 NR, NI, NQ, J

      REAL*8    ANRM, TMP, EQ
      REAL*8    ONE /1.D0/, TWO /2.D0/, SIX /6.D0/
      REAL*8    TWOL /0.69314718055994530941723212145818D0/
      INTEGER*4 NB, MM, NQC
      REAL*8    DINFNM
      INTEGER*4 FLOOR, CEILING
      REAL*8    TINY

      NI = M * 2
      MM = M * M

      ANRM = DINFNM (M, M, A, LDA) 
      NQ = 1
      EQ = ONE / SIX
      DO 10 WHILE (.TRUE.)
          TMP = EQ * ANRM
          IF (TMP * DEXP (TMP) .LE. TOL .AND. NQ .GT. 1) GOTO 15
          NQ = NQ + 1
          EQ = EQ / DFLOAT (16 * (4 * NQ * NQ - 1))
10    CONTINUE

15    J = 0
      IF (ANRM .GT. TINY (ONE)) THEN
          J = 1 + CEILING (DLOG (ANRM) / TWOL)
      ENDIF

      NQC = FLOOR (DFLOAT (NQ) / TWO) + 1
      NB  = FLOOR (DFLOAT (NQC - 1) /
     1             DFLOAT (CEILING (DSQRT (DFLOAT (NQC - 1)))))
      NB  = (2 * NB + 2) * MM

      NR  = 4 * MM + 2 * NQC + NB + 5 * M
      RETURN
      END !SUBROUTINE DMEXPC



      SUBROUTINE DPOLY2 (M, A, N, V1, V2, P1, P2, B, B2)
      INTEGER*4   M, N
      REAL*8      A(M*M), V1(N), V2(N), P1(M*M), P2(M*M)
      REAL*8      B(1), B2(1)
      INTEGER*4   I, K, NS, NR, NQSR, MM, NRM
      REAL*8      Q, ONE /1.D0/, ZERO /0.D0/
      CHARACTER*1 TRANS /'N'/
      INTEGER*4   FLOOR, CEILING

      MM = M * M
      DO 10 I = 1, MM, M + 1
          P1(I) = V1(1)
          P2(I) = V2(1)
10    CONTINUE

      Q    = DFLOAT  (N - 1)        ! b(0) is V(1)
      NS   = CEILING (DSQRT (Q))
      NR   = FLOOR   (Q / DFLOAT (NS))
      NQSR = N - 1 - NS * NR        ! q - sr
      NRM  = (NR + 1) * MM + 1      ! the 1st el. of the last matrix

      CALL DCOPY (MM, A, 1, B, 1)   ! the first of these matrices
                                    ! will contain powers of A
      DO 20 K = 1, NR
          DO 30 I = 1, MM, M + 1
              B(MM * K + I)        = V1(NS * K + 1)  ! b(i)*I
              B2(MM * (K - 1) + I) = V2(NS * K + 1)  ! b(i)*I
30        CONTINUE
20    CONTINUE

      DO 40 I = 1, NS - 1
          CALL DAXPY (MM, V1(I + 1), B, 1, P1, 1)
          CALL DAXPY (MM, V2(I + 1), B, 1, P2, 1)

          DO 50 K = 1, NR - 1
              CALL DAXPY (MM, V1(NS * K + I + 1), B, 1,
     1                                        B(K * MM + 1), 1)
              CALL DAXPY (MM, V2(NS * K + I + 1), B, 1,
     1                                        B2((K - 1) * MM + 1), 1)
50        CONTINUE

          IF (I .LE. NQSR) THEN
              CALL DAXPY (MM, V1(NS * NR + I + 1), B, 1,
     1                                        B(NR * MM + 1), 1)
              CALL DAXPY (MM, V2(NS * NR + I + 1), B, 1,
     1                                        B2((NR - 1) * MM + 1), 1)
          ENDIF

          CALL DCOPY (MM, B, 1, B(NRM), 1)
          CALL DGEMM (TRANS, TRANS, M, M, M, ONE, B(NRM), M, A, M,
     1                                           ZERO, B, M)
40    CONTINUE

      CALL DGEMM (TRANS, TRANS, M, M, M, ONE, B(MM + 1), M,
     1                                           B, M, ONE, P1, M)
      CALL DGEMM (TRANS, TRANS, M, M, M, ONE, B2, M,
     1                                           B, M, ONE, P2, M)

      IF (NR .GT. 1) CALL DCOPY (MM, B, 1, B(NRM), 1)

      DO 60 K = 2, NR
          CALL DCOPY (MM, B, 1, B(MM + 1), 1)
          CALL DGEMM (TRANS, TRANS, M, M, M, ONE, B(MM + 1), M,
     1                                           B(NRM), M, ZERO, B, M)
          CALL DGEMM (TRANS, TRANS, M, M, M, ONE,
     1                      B(K * MM + 1), M, B, M, ONE, P1, M)
          CALL DGEMM (TRANS, TRANS, M, M, M, ONE,
     1                      B2((K - 1) * MM + 1), M, B, M, ONE, P2, M)
60    CONTINUE

      RETURN
      END !SUBROUTINE DPOLY2
