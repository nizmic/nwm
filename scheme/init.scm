;;; Sample init.scm file for nwm
;;;
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

; number of "master" windows
(define master-count 1)

; simple window tiling hook
(define (arrange-hook)
  (log "*** in arrange-hook ***")
  (let ((s-h (screen-height))
	(s-w (screen-width))
	(clients (all-clients))
	(client-count (length (all-clients))))
    (log (format #f "screen is ~Ax~A" s-w s-h))
    (log (format #f "arranging ~A clients" (length clients)))
    (for-each (lambda (c)
		(log (format #f "client is ~Ax~A" (client-width c) (client-height c))))
	      clients)
    ; arrange master windows
    (do ((idx 0 (+ idx 1))
	 (width (floor (/ s-w 2)))
	 (height (floor (/ s-h master-count)))
	 (x 0))
	((= idx master-count))
      (let ((client (list-ref clients idx))
	    (y (* idx height)))
	(log (format #f "setting master #~A ~A to (~A,~A) ~Ax~A"
		     idx
		     client
		     x y width height))
	(move-client client x y)
	(resize-client client width height)
	(map-client client)))
    ; arrange other windows
    (do ((idx master-count (+ idx 1))
	 (width (floor (/ s-w 2)))
	 (height (floor (/ s-h (- client-count master-count))))
	 (x (floor (/ s-w 2))))
	((= idx client-count))
      (let ((client (list-ref clients idx))
	    (y (* (- idx master-count) height)))
	(log (format #f "setting ~A to (~A,~A) ~Ax~A"
		     client
		     x y width height))
	(move-client client x y)
	(resize-client client width height)
	(map-client client)))))
