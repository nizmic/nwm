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

; load the auto-tiling routines
(load "auto-tile.scm")

(define border-color #x6CA0A3)

; path to terminal program
(define term-program '("xterm"))
; you can supply arguments like so:
; (define term-program '("xterm" "-e" "screen"))

; number of "master" windows in auto-tiling routines
(define master-count 1)

; the percent of the screen width dedicated to the master windows
(define master-width 50)

; define the list of window arrangements you would like to use
; (defined externally in auto-tile.scm
(define arrangements (list auto-vtile auto-htile))
(define (arrange-clients) ((car arrangements)))

(add-hook! map-client-hook focus-client)
(add-hook! map-client-hook (lambda (client)
                             (arrange-clients)))

(add-hook! unmap-client-hook (lambda (client)
                               (focus-client (next-client client))))
(add-hook! unmap-client-hook (lambda (client)
                               (arrange-clients)))

(add-hook! destroy-client-hook (lambda (client)
                                 (focus-client (next-client client))))
(add-hook! destroy-client-hook (lambda (client)
                                 (arrange-clients)))

(add-hook! focus-client-hook draw-border)

; cycle through the arrangements
(define (cycle-arrangement)
  (begin
    (set! arrangements (append (cdr arrangements)
                               (list (car arrangements))))
    (set! arrange-clients (lambda () ((car arrangements))))
    (arrange-clients)
    (draw-border (get-focus-client))))

(define (add-master)
  (set! master-count (+ master-count 1))
  (arrange-clients)
  (draw-border (get-focus-client)))

(define (remove-master)
  (if (> master-count 1)
      (set! master-count (- master-count 1)))
  (arrange-clients)
  (draw-border (get-focus-client)))

(define (grow-master)
  (if (< master-perc 94)
      (set! master-perc (+ master-perc 5)))
  (arrange-clients)
  (draw-border (get-focus-client)))

(define (shrink-master)
  (if (>= master-perc 6)
      (set! master-perc (- master-perc 5)))
  (arrange-clients)
  (draw-border (get-focus-client)))

(define (reverse-clients)
  (client-list-reverse)
  (arrange-clients)
  (draw-border (get-focus-client)))

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

; cycle arrangements, ctrl-shift-space
(bind-key 5 "Space" cycle-arrangement)

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

; swap master window, ctrl-s
(bind-key 5 "s" (lambda ()
                  (begin
                    (swap-master)
                    (arrange-clients))))

