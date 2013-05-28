;;; Sample init.scm file for nwm
;;;
;;; Copyright (C) 2013  Brandon Invergo
;;; Copyright (C) 2010-2012  Nathan Sullivan
;;;
;;; This program is free software; you can redistribute it and/or 
;;; modify it under the terms of the GNU General Public License 
;;; as published by the Free Software Foundation; either version 2 
;;; of the License, or (at your option) any later version. 
;;;
;;; This program is distributed in the hope that it will be useful, 
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of 
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
;;; GNU General Public License for more details. 
;;;
;;; You should have received a copy of the GNU General Public License 
;;; along with this program; if not, write to the Free Software 
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
;;; 02110-1301, USA 
;;;

; path to terminal program
(define term-program '("xterm"))
; you can supply arguments like so:
; (define term-program '("xterm" "-e" "screen"))

; number of "master" windows
(define master-count 1)

; the percent of the screen width dedicated to the master windows
(define master-width 50)

(define (arrange-client client x y width height)
  (move-client client x y)
  (resize-client client width height)
  (map-client client))

(define (split-vertical-iter clients x width increment cur)
  (arrange-client (car clients) x (+ cur 1) width (- increment 2))
  (if (= (length clients) 1)
      #t
      (split-vertical-iter (cdr clients) x width increment (+ cur increment))))

(define (split-vertical clients x width)
  (if (> (length clients) 0)
      (let ((increment (floor (/ (screen-height) (length clients)))))
        (split-vertical-iter clients x width increment 0))))

(define (arrange-hook)
  (let ((clients (all-clients))
	(client-count (length (all-clients)))
	(master-screen-width (floor (* (screen-width) (/ master-width 100)))))
    (cond
     ((= client-count 1) (arrange-client (car clients)
					 1 1
					 (- (screen-width) 2)
					 (- (screen-height) 2)))
     ((> client-count 1)
      (split-vertical
       (list-head clients (min client-count master-count))
       1 (- master-screen-width 2))
      (split-vertical
       (list-tail clients (min client-count master-count))
       (+ master-screen-width 1) (- (- (screen-width) master-screen-width) 2))))))

(define (add-master)
  (set! master-count (+ master-count 1))
  (arrange-hook))

(define (remove-master)
  (if (> master-count 1)
      (set! master-count (- master-count 1)))
  (arrange-hook))

(define (grow-master)
  (if (< master-width 94)
      (set! master-width (+ master-width 5)))
  (arrange-hook))

(define (shrink-master)
  (if (>= master-width 6)
      (set! master-width (- master-width 5)))
  (arrange-hook))

(define (focus-next)
  (focus-client (next-client (get-focus-client))))

(define (focus-prev)
  (focus-client (prev-client (get-focus-client))))

(define (close)
  (destroy-client (get-focus-client)))

(define (launch-term)
  (launch-program term-program))

; run arrange-hook when hit ctrl-spacebar
(bind-key 4 "Space" arrange-hook)

; add a master, ctrl-;
(bind-key 4 ";" add-master)

; remove a master, ctrl-'
(bind-key 4 "'" remove-master)

; focus next, ctrl-j
(bind-key 4 "j" focus-next)

; focus prev, ctrl-k
(bind-key 4 "k" focus-prev)

; focus next, ctrl-l
(bind-key 4 "l" grow-master)

; focus prev, ctrl-h
(bind-key 4 "h" shrink-master)

; launch terminal, ctrl-enter
(bind-key 4 "Enter" launch-term)

; quit nwm, ctrl-shift-q
(bind-key 5 "q" nwm-stop)

; close window, ctrl-shift-c
(bind-key 5 "c" close)
